/*
 * Copyright (C) 2005 to 2014 by Jonathan Duddington
 * email: jonsd@users.sourceforge.net
 * Copyright (C) 2015-2017 Reece H. Dunn
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>.
 */


// Part of the Engine: translating a clause of text to phonemes (espeak-ng's translate.c): the clause loop that splits the text into words,
// handles numbers, language switches and punctuation, the per-word step that turns a word into phoneme codes (TranslateWord2), and the
// translators for second and third languages.

#include "engine.h"

namespace propan2ol {

static const unsigned short breaks[] = { '_', 0 };

void Engine::DeleteTranslator( Translator* tr ) {
	if ( !tr ) {
		return;
	}

	if ( tr->data_dictlist != nullptr ) {
		free( tr->data_dictlist );
	}
	free( tr );
}

int Engine::lookupwchar( const unsigned short* list, int c ) {
	// Is the character c in the list ?
	int ix;

	for ( ix = 0; list[ix] != 0; ix++ ) {
		if ( list[ix] == c ) {
			return ix + 1;
		}
	}
	return 0;
}

char* Engine::strchr_w( const char* s, int c ) {
	// return nullptr for any non-ascii character
	if ( c >= 0x80 ) {
		return nullptr;
	}
	return strchr( (char*)s, c ); // (char *) is needed for Borland compiler
}

int Engine::TranslateWord( Translator* tr, char* word_start, WORD_TAB* wtab, char* word_out ) {
	char words_phonemes[N_WORD_PHONEMES]; // a word translated into phoneme codes
	char* phonemes = words_phonemes;


	int flags = TranslateWord3( tr, word_start, wtab, word_out, &any_stressed_words, current_alphabet, word_phonemes, sizeof( word_phonemes ) );
	if ( flags & FLAG_TEXTMODE && word_out ) {
		// Ensure that start of word rules match with the replaced text,
		// so that emoji and other characters are pronounced correctly.
		char word[N_WORD_BYTES + 1];
		word[0] = 0;
		word[1] = ' ';
		strcpy( word + 2, word_out );
		word_out = word + 2;

		bool first_word = true;
		int available = N_WORD_PHONEMES;
		while ( *word_out && available > 1 ) {
			int c;
			utf8_in( &c, word_out );
			if ( chars::is_upper( c ) ) {
				wtab->flags |= FLAG_FIRST_UPPER;
				utf8_out( tolower( c ), word_out );
			} else {
				wtab->flags &= ~FLAG_FIRST_UPPER;
			}

			// dictionary_skipwords is a global variable and TranslateWord3 will reset it to 0 at the beginning.
			// However, dictionary_skipwords value is still needed outside this scope.
			// So we backup and restore it at the end of this scope.
			int skipwords = dictionary_skipwords;
			TranslateWord3( tr, word_out, wtab, nullptr, &any_stressed_words, current_alphabet, word_phonemes, sizeof( word_phonemes ) );

			int n;
			if ( first_word ) {
				n = snprintf( phonemes, available, "%s", word_phonemes );
				first_word = false;
			} else {
				n = snprintf( phonemes, available, "%c%s", phonEND_WORD, word_phonemes );
			}

			available -= n;
			phonemes += n;

			// skip to the next word in a multi-word replacement. Always skip at least one word.
			for ( dictionary_skipwords++; dictionary_skipwords > 0; dictionary_skipwords-- ) {
				while ( !isspace( *word_out ) ) {
					++word_out;
				}
				while ( isspace( *word_out ) ) {
					++word_out;
				}
			}
			dictionary_skipwords = skipwords;
		}

		// If the list file contains a text replacement to another
		// entry in the list file, e.g.:
		//     ripost     riposte $text
		//     riposte    rI#p0st
		// calling it from a prefix or suffix rule such as 'riposted'
		// causes word_out[0] to be nullptr, as TranslateWord3 has the
		// information needed to perform the mapping. In this case,
		// no phonemes have been written in this loop and the phonemes
		// have been calculated, so don't override them.
		if ( phonemes != words_phonemes ) {
			snprintf( word_phonemes, sizeof( word_phonemes ), "%s", words_phonemes );
		}
	}
	return flags;
}

void Engine::SetPlist2( PHONEME_LIST2* p, unsigned char phcode ) {
	p->phcode = phcode;
	p->stresslevel = 0;
	p->tone_ph = 0;
	p->synthflags = 0;
	p->sourceix = 0;
}

int Engine::CountSyllables( unsigned char* phonemes ) {
	int count = 0;
	int phon;
	while ( ( phon = *phonemes++ ) != 0 ) {
		if ( phoneme_tab[phon]->type == phVOWEL ) {
			count++;
		}
	}
	return count;
}

int Engine::SetAlternateTranslator( const char* new_language, Translator** translator, char translator_language[20] ) {
	// Set alternate translator to a second language
	int new_phoneme_tab;

	if ( ( new_phoneme_tab = SelectPhonemeTableName( new_language ) ) >= 0 ) {
		if ( ( *translator != nullptr ) && ( strcmp( new_language, translator_language ) != 0 ) ) {
			// we already have an alternative translator, but not for the required language, delete it
			DeleteTranslator( *translator );
			*translator = nullptr;
		}

		if ( *translator == nullptr ) {
			*translator = SelectTranslator( new_language );
			strcpy( translator_language, new_language );

			if ( LoadDictionary( *translator, ( *translator )->dictionary_name, 0 ) != 0 ) {
				SelectPhonemeTable( voice_phoneme_tab_ix ); // revert to original phoneme table
				new_phoneme_tab = -1;
				translator_language[0] = 0;
			}
			( *translator )->phoneme_tab_ix = new_phoneme_tab;
		}
	}
	if ( *translator != nullptr ) {
		( *translator )->phonemes_repeat[0] = 0;
	}
	return new_phoneme_tab;
}

int Engine::SetTranslator2( const char* new_language ) { return SetAlternateTranslator( new_language, &translator2, translator2_language ); }

int Engine::SetTranslator3( const char* new_language ) { return SetAlternateTranslator( new_language, &translator3, translator3_language ); }

int Engine::TranslateWord2( Translator* tr, char* word, WORD_TAB* wtab, int pre_pause ) {
	WordPhonemes wp;

	int len = wtab->length;
	if ( len > 31 ) {
		len = 31;
	}
	wp.source_ix = ( wtab->sourceix & 0x7ff ) | ( len << 11 ); // bits 0-10 sourceix, bits 11-15 word length

	wp.word_flags = wtab[0].flags;

	if ( n_ph_list2 >= N_PHONEME_LIST - 2 ) {
		// No room, can't translate anything
		return 0;
	}

	if ( ( word[0] == 0 ) || ( wp.word_flags & FLAG_DELETE_WORD ) ) {
		// nothing to translate
		word_phonemes[0] = 0;
		return 0;
	}

	if ( n_ph_list2 >= N_PHONEME_LIST - 7 - 2 ) {
		// We may require up to 7 phonemes, plus the 2 phonemes from the caller, can't translate safely
		return 0;
	}

	// after a $pause word attribute, ignore a $pause attribute on the next two words
	if ( tr->prepause_timeout > 0 ) {
		tr->prepause_timeout--;
	}

	if ( std::optional<int> result = WordToPhonemeCodes( tr, word, wtab, pre_pause, wp ) ) {
		return *result;
	}
	AddPauses( tr, pre_pause );
	return AddWordPhonemes( tr, wp );
}

// The word's phonemes as phoneme codes in word_phonemes (wp.p): from the phoneme mnemonics it is given in, or through the dictionary and the
// rules, a second language's if the word belongs to one. Also sets pre_pause from the word's flags. Returns the final result if the word
// is to be redone as individual letters or the language switch fails, else nothing.
std::optional<int> Engine::WordToPhonemeCodes( Translator* tr, char* word, WORD_TAB* wtab, int& pre_pause, WordPhonemes& wp ) {
	char word_copy[N_WORD_BYTES + 1];
	char word_replaced[N_WORD_BYTES + 1];
	int bad_phoneme;

	wp.p = (unsigned char*)word_phonemes;
	if ( wp.word_flags & FLAG_PHONEMES ) {
		// The input is in phoneme mnemonics, not language text

		if ( memcmp( word, "_^_", 3 ) == 0 ) {
			SwitchLanguage( word, word_phonemes );
		} else {
			EncodePhonemes( word, word_phonemes, &bad_phoneme );
		}

		wp.flags = FLAG_FOUND;
	} else {
		int c2;
		int ix = 0;
		int word_copy_len;
		while ( ( ( c2 = word_copy[ix] = word[ix] ) != ' ' ) && ( c2 != 0 ) && ( ix < N_WORD_BYTES ) ) {
			ix++;
		}
		word_copy_len = ix;

		word_replaced[2] = 0;
		wp.flags = TranslateWord( translator, word, wtab, &word_replaced[2] );

		if ( wp.flags & FLAG_SPELLWORD ) {
			// re-translate the word as individual letters, separated by spaces
			memcpy( word, word_copy, word_copy_len );
			return wp.flags;
		}

		if ( ( wp.flags & FLAG_COMBINE ) && !( wtab[1].flags & FLAG_PHONEMES ) ) {
			CombineFlag( tr, wtab, word, &wp.flags, wp.p, word_phonemes );
		}

		if ( wp.p[0] == phonSWITCH ) {
			int switch_attempt;
			strcpy( wp.old_dictionary_name, dictionary_name );
			for ( switch_attempt = 0; switch_attempt < 2; switch_attempt++ ) {
				// this word uses a different language
				memcpy( word, word_copy, word_copy_len );

				const char* new_language;
				new_language = (char*)( &wp.p[1] );
				if ( new_language[0] == 0 ) {
					new_language = ESPEAKNG_DEFAULT_VOICE;
				}

				wp.switch_phonemes = SetTranslator2( new_language );

				if ( wp.switch_phonemes >= 0 ) {
					// re-translate the word using the new translator
					wtab[0].flags |= FLAG_TRANSLATOR2;
					if ( word_replaced[2] != 0 ) {
						word_replaced[0] = 0; // byte before the start of the word
						word_replaced[1] = ' ';
						wp.flags = TranslateWord( translator2, &word_replaced[1], wtab, nullptr );
					} else {
						wp.flags = TranslateWord( translator2, word, wtab, &word_replaced[2] );
					}
				}

				if ( wp.p[0] != phonSWITCH ) {
					break;
				}
			}

			if ( wp.p[0] == phonSWITCH ) {
				return FLAG_SPELLWORD;
			}

			if ( wp.switch_phonemes < 0 ) {
				// language code is not recognised or 2nd translator won't translate it
				wp.p[0] = phonSCHWA; // just say something
				wp.p[1] = phonSCHWA;
				wp.p[2] = 0;
			}

			if ( wp.switch_phonemes == -1 ) {
				strcpy( dictionary_name, wp.old_dictionary_name );
				SelectPhonemeTable( voice_phoneme_tab_ix );

				// leave switch_phonemes set, but use the original phoneme table number.
				// This will suppress LOPT_REGRESSIVE_VOICING
				wp.switch_phonemes = voice_phoneme_tab_ix; // original phoneme table
			}
		}

		if ( !( wp.word_flags & FLAG_HYPHEN ) ) {
			if ( wp.flags & FLAG_PAUSE1 ) {
				if ( pre_pause < 1 ) {
					pre_pause = 1;
				}
			}
			if ( ( wp.flags & FLAG_PREPAUSE ) && !( wp.word_flags & ( FLAG_LAST_WORD | FLAG_FIRST_WORD ) ) && !( wtab[-1].flags & FLAG_FIRST_WORD ) &&
				 ( tr->prepause_timeout == 0 ) ) {
				// the word is marked in the dictionary list with $pause
				if ( pre_pause < 4 ) {
					pre_pause = 4;
				}
				tr->prepause_timeout = 3;
			}
		}
	}
	return std::nullopt;
}

// Pause phonemes before the word, from punctuation (brackets or quotes) in the text, or because the dictionary marks the word as a
// conjunction.
void Engine::AddPauses( Translator* tr, int& pre_pause ) {
	// Each iteration may require up to 1 phoneme
	// and after this loop we may require up to 7 phonemes
	// and our caller requires 2 phonemes
	while ( ( pre_pause > 0 ) && ( n_ph_list2 < N_PHONEME_LIST - 7 - 2 ) ) {
		// add pause phonemes here. Either because of punctuation (brackets or quotes) in the
		// text, or because the word is marked in the dictionary lookup as a conjunction
		if ( pre_pause > 1 ) {
			SetPlist2( &ph_list2[n_ph_list2++], phonPAUSE );
			pre_pause -= 2;
		} else {
			SetPlist2( &ph_list2[n_ph_list2++], phonPAUSE_NOLINK );
			pre_pause--;
		}
		tr->end_stressed_vowel = 0; // forget about the previous word
		tr->prev_dict_flags[0] = 0;
		tr->prev_dict_flags[1] = 0;
	}
}

// Adds the word's phonemes to the clause's first-stage phoneme list (ph_list2): the language switch around a word of another language,
// the stress of each vowel, tones, syllabic consonants and lengthening. Returns the word's dictionary flags.
int Engine::AddWordPhonemes( Translator* tr, WordPhonemes& wp ) {
	int stress;
	int next_stress;
	int next_tone = 0;
	int srcix;
	int found_dict_flag;
	unsigned char ph_code;
	PHONEME_LIST2* plist2;
	PHONEME_TAB* ph;
	int max_stress;
	int max_stress_ix = 0;
	int prev_vowel = -1;
	bool first_phoneme = true;

	stress = 0;
	next_stress = 1;
	srcix = 0;
	max_stress = -1;

	found_dict_flag = 0;
	if ( ( wp.flags & FLAG_FOUND ) && !( wp.flags & FLAG_TEXTMODE ) ) {
		found_dict_flag = SFLAG_DICTIONARY;
	}

	plist2 = &ph_list2[n_ph_list2];
	// From here we may require up to 4+1+3 phonemes

	// This may require up to 4 phonemes

	// This may require up to 1 phoneme
	if ( wp.switch_phonemes >= 0 ) {
		if ( ( wp.p[0] == phonPAUSE ) && ( wp.p[1] == phonSWITCH ) ) {
			// the new word starts with a phoneme table switch, so there's no need to switch before it.
			if ( ph_list2[n_ph_list2 - 1].phcode == phonSWITCH ) {
				// previous phoneme is also a phonSWITCH, delete it
				n_ph_list2--;
			}
		} else {
			// this word uses a different phoneme table
			if ( ph_list2[n_ph_list2 - 1].phcode == phonSWITCH ) {
				// previous phoneme is also a phonSWITCH, just change its phoneme table number
				n_ph_list2--;
			} else {
				SetPlist2( &ph_list2[n_ph_list2], phonSWITCH );
			}
			ph_list2[n_ph_list2++].tone_ph = wp.switch_phonemes; // temporary phoneme table number
		}
	}

	// remove initial pause from a word if it follows a hyphen
	if ( ( wp.word_flags & FLAG_HYPHEN ) && ( phoneme_tab[*wp.p]->type == phPAUSE ) ) {
		wp.p++;
	}


	// Each iteration may require up to 1 phoneme
	// and after this loop we may require up to 3 phonemes
	// and our caller requires 2 phonemes
	while ( ( ( ph_code = *wp.p++ ) != 0 ) && ( n_ph_list2 < N_PHONEME_LIST - 3 - 2 ) ) {
		if ( ph_code == 255 ) {
			continue; // unknown phoneme
		}

		// Add the phonemes to the first stage phoneme list (ph_list2)
		ph = phoneme_tab[ph_code];
		if ( ph == nullptr ) {
			printf( "Invalid phoneme code %d\n", ph_code );
			continue;
		}

		if ( ph_code == phonSWITCH ) {
			ph_list2[n_ph_list2].phcode = ph_code;
			ph_list2[n_ph_list2].stresslevel = 0;
			ph_list2[n_ph_list2].sourceix = 0;
			ph_list2[n_ph_list2].synthflags = 0;
			ph_list2[n_ph_list2++].tone_ph = *wp.p;
			SelectPhonemeTable( *wp.p );
			wp.p++;
		} else if ( ph->type == phSTRESS ) {
			// don't add stress phonemes codes to the list, but give their stress
			// value to the next vowel phoneme
			// std_length is used to hold stress number or (if >10) a tone number for a tone language
			if ( ph->program == 0 ) {
				next_stress = ph->std_length;
			} else {
				// for tone languages, the tone number for a syllable follows the vowel
				if ( prev_vowel >= 0 ) {
					ph_list2[prev_vowel].tone_ph = ph_code;
				} else {
					next_tone = ph_code; // no previous vowel, apply to the next vowel
				}
			}
		} else if ( ph_code == phonSYLLABIC ) {
			// mark the previous phoneme as a syllabic consonant
			prev_vowel = n_ph_list2 - 1;
			ph_list2[prev_vowel].synthflags |= SFLAG_SYLLABLE;
			ph_list2[prev_vowel].stresslevel = next_stress;
		} else if ( ph_code == phonLENGTHEN ) {
			ph_list2[n_ph_list2 - 1].synthflags |= SFLAG_LENGTHEN;
		} else if ( ph_code == phonEND_WORD ) {
			// a || symbol in a phoneme string was used to indicate a word boundary
			// Don't add this phoneme to the list, but make sure the next phoneme has
			// a newword indication
			srcix = wp.source_ix + 1;
		} else if ( ph_code == phonX1 ) {
			// a language specific action
			wp.flags |= FLAG_DOUBLING;
		} else {
			ph_list2[n_ph_list2].phcode = ph_code;
			ph_list2[n_ph_list2].tone_ph = 0;
			ph_list2[n_ph_list2].synthflags = found_dict_flag;
			ph_list2[n_ph_list2].sourceix = srcix;
			srcix = 0;

			if ( ph->type == phVOWEL ) {
				stress = next_stress;
				next_stress = 1; // default is 'unstressed'

				if ( stress >= 4 ) {
					any_stressed_words = true;
				}

				if ( ( prev_vowel >= 0 ) && ( n_ph_list2 - 1 ) != prev_vowel ) {
					ph_list2[n_ph_list2 - 1].stresslevel = stress; // set stress for previous consonant
				}

				ph_list2[n_ph_list2].synthflags |= SFLAG_SYLLABLE;
				prev_vowel = n_ph_list2;

				if ( stress > max_stress ) {
					max_stress = stress;
					max_stress_ix = n_ph_list2;
				}
				if ( next_tone != 0 ) {
					ph_list2[n_ph_list2].tone_ph = next_tone;
					next_tone = 0;
				}
			} else {
				if ( first_phoneme && tr->prev_dict_flags[0] & FLAG_DOUBLING ) {
					// double the initial consonant if the previous word is marked with a flag
					ph_list2[n_ph_list2].synthflags |= SFLAG_LENGTHEN;
				}
			}

			ph_list2[n_ph_list2].stresslevel = stress;
			n_ph_list2++;
			first_phoneme = false;
		}
	}
	// From here, we may require up to 3 phonemes

	// This may require up to 1 phoneme
	if ( wp.word_flags & FLAG_COMMA_AFTER ) {
		SetPlist2( &ph_list2[n_ph_list2++], phonPAUSE_CLAUSE );
	}

	// don't set new-word if there is a hyphen before it
	if ( ( wp.word_flags & FLAG_HYPHEN ) == 0 ) {
		plist2->sourceix = wp.source_ix;
	}

	tr->end_stressed_vowel = 0;
	if ( ( stress >= 4 ) && ( phoneme_tab[ph_list2[n_ph_list2 - 1].phcode]->type == phVOWEL ) ) {
		tr->end_stressed_vowel = 1; // word ends with a stressed vowel
	}

	// This may require up to 1 phoneme
	if ( wp.switch_phonemes >= 0 ) {
		// this word uses a different phoneme table, now switch back
		strcpy( dictionary_name, wp.old_dictionary_name );
		SelectPhonemeTable( voice_phoneme_tab_ix );
		SetPlist2( &ph_list2[n_ph_list2], phonSWITCH );
		ph_list2[n_ph_list2++].tone_ph = voice_phoneme_tab_ix; // original phoneme table number
	}


	if ( wp.flags & FLAG_STRESS_END2 ) {
		// this's word's stress could be increased later
		ph_list2[max_stress_ix].synthflags |= SFLAG_PROMOTE_STRESS;
	}

	tr->prev_dict_flags[0] = wp.flags;
	return wp.flags;
}


const char* Engine::FindReplacementChars( Translator* tr, const char** pfrom, unsigned int c, const char* next, int* ignore_next_n ) {
	const char* from = *pfrom;
	while ( !is_str_totally_null( from, 4 ) ) {
		unsigned int fc = 0; // from character
		unsigned int nc = c; // next character
		const char* match_next = next;

		*pfrom = from;

		from += utf8_in( (int*)&fc, from );
		if ( nc == fc ) {
			if ( *from == 0 ) {
				return from + 1;
			}

			bool matched = true;
			int nmatched = 0;
			while ( *from != 0 ) {
				from += utf8_in( (int*)&fc, from );

				match_next += utf8_in( (int*)&nc, match_next );
				nc = towlower2( nc, tr );

				if ( nc != fc ) {
					matched = false;
				} else {
					nmatched++;
				}
			}

			if ( matched ) {
				*ignore_next_n = nmatched;
				return from + 1;
			}
		}

		// replacement 'from' string (skip the remaining part, if any)
		while ( *from != '\0' ) {
			from++;
		}
		from++;

		// replacement 'to' string
		while ( *from != '\0' ) {
			from++;
		}
		from++;
	}
	return nullptr;
}

// handle .replace rule in xx_rules file
int Engine::SubstituteChar( Translator* tr, unsigned int c, unsigned int next_in, const char* next, int* insert, int* wordflags ) {
	unsigned int new_c, c2 = ' ', c_lower;
	int upper_case = 0;
	if ( ignore_next_n > 0 ) {
		ignore_next_n--;
		return 8;
	}

	if ( c == 0 ) {
		return 0;
	}

	const char* from = (const char*)tr->langopts.replace_chars;
	if ( from == nullptr ) {
		return c;
	}

	// there is a list of character codes to be substituted with alternative codes

	if ( chars::is_upper( c_lower = c ) ) {
		c_lower = towlower2( c, tr );
		upper_case = 1;
	}

	const char* to = FindReplacementChars( tr, &from, c_lower, next, &ignore_next_n );
	if ( to == nullptr ) {
		return c; // no substitution
	}


	to += utf8_in( (int*)&new_c, to );
	if ( *to != 0 ) {
		// there is a second character to be inserted
		// don't convert the case of the second character unless the next letter is also upper case
		to += utf8_in( (int*)&c2, to );
		if ( upper_case && chars::is_upper( next_in ) ) {
			c2 = chars::ucd_to_upper( c2 );
		}
		*insert = c2;
	}

	if ( upper_case ) {
		new_c = chars::ucd_to_upper( new_c );
	}

	*wordflags |= FLAG_CHAR_REPLACED;
	return new_c;
}

int Engine::TranslateChar( Translator* tr, char* ptr, int prev_in, unsigned int c, unsigned int next_in, int* insert, int* wordflags ) {
	// To allow language specific examination and replacement of characters

	int code;
	int next2;

	static const unsigned char hangul_compatibility[0x34] = { 0,	0x00, 0x01, 0xaa, 0x02, 0xac, 0xad, 0x03, 0x04, 0x05, 0xb0, 0xb1, 0xb2,
															  0xb3, 0xb4, 0xb4, 0xb6, 0x06, 0x07, 0x08, 0xb9, 0x09, 0x0a, 0xbc, 0x0c, 0x0d,
															  0x0e, 0x0f, 0x10, 0x11, 0x12, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
															  0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75 };

	// check for Korean Hangul letters
	if ( ( ( code = c - 0xac00 ) >= 0 ) && ( c <= 0xd7af ) ) {
		// break a syllable hangul into 2 or 3 individual jamo
		int initial = ( code / 28 ) / 21;
		int medial = ( code / 28 ) % 21;
		int final = code % 28;

		if ( initial == 11 ) {
			// null initial
			c = medial + 0x1161;
			if ( final > 0 ) {
				*insert = final + 0x11a7;
			}
		} else {
			// extract the initial and insert the remainder with a null initial
			c = initial + 0x1100;
			*insert = ( 11 * 28 * 21 ) + ( medial * 28 ) + final + 0xac00;
		}
		return c;
	} else if ( ( ( code = c - 0x3130 ) >= 0 ) && ( code < 0x34 ) ) {
		// Hangul compatibility jamo
		return hangul_compatibility[code] + 0x1100;
	}

	switch ( tr->translator_name ) {
	case L( 'a', 'f' ):
	case L( 'n', 'l' ):
		// look for 'n  and replace by a special character (unicode: schwa)

		if ( ( c == '\'' ) && !chars::is_alpha( prev_in ) ) {
			utf8_in( &next2, &ptr[1] );

			if ( IsSpace( next2 ) ) {
				if ( ( next_in == 'n' ) && ( tr->translator_name == L( 'a', 'f' ) ) ) {
					// n preceded by either apostrophe or U2019 "right single quotation mark"
					ptr[0] = ' ';  // delete the n
					return 0x0259; // replace  '  by  unicode schwa character
				}
				if ( ( next_in == 'n' ) || ( next_in == 't' ) ) {
					// Dutch, [@n] and [@t]
					return 0x0259; // replace  '  by  unicode schwa character
				}
			}
		}
		break;
	}
	// handle .replace rule in xx_rules file
	return SubstituteChar( tr, c, next_in, ptr, insert, wordflags );
}

static const char* const UCase_ga[] = { "bp", "bhf", "dt", "gc", "hA", "mb", "nd", "ng", "ts", "tA", "nA", nullptr };

int Engine::UpperCaseInWord( Translator* tr, char* word, int c ) {
	if ( tr->translator_name == L( 'g', 'a' ) ) {
		int ix;
		const char* p;

		for ( ix = 0;; ix++ ) {
			int len;
			if ( ( p = UCase_ga[ix] ) == nullptr ) {
				break;
			}

			len = strlen( p );
			if ( ( word[-len] == ' ' ) && ( memcmp( &word[-len + 1], p, len - 1 ) == 0 ) ) {
				if ( ( c == p[len - 1] ) || ( ( p[len - 1] == 'A' ) && IsVowel( tr, c ) ) ) {
					return 1;
				}
			}
		}
	}
	return 0;
}

// Translates the next clause of the text: reads it (ReadClause), scans it into normalised words, translates each word to phonemes, and
// makes the clause's phoneme list. `tone_out` gets the clause's tone type, `terminator_out` its end (full stop, comma...).
void Engine::TranslateClauseWithTerminator( Translator* tr, int* tone_out, char** voice_change, int* terminator_out ) {
	if ( tr == nullptr ) {
		return;
	}

	ClauseScan s;
	BeginClause( tr, s, tone_out, terminator_out );
	ScanWords( tr, s );
	FinishWordList( tr, s );
	TranslateWords( tr, s, tone_out );

	for ( int i = 0; i < 2; i++ ) {
		// terminate the clause with 2 PAUSE phonemes
		PHONEME_LIST2* p2 = &ph_list2[n_ph_list2 + i];
		p2->phcode = phonPAUSE;
		p2->stresslevel = 0;
		p2->sourceix = s.source_index;
		p2->synthflags = 0;
	}
	n_ph_list2 += 2;

	if ( Eof() ) {
		s.clause_pause = 10;
	}

	MakePhonemeList( tr, s.clause_pause, s.new_sentence2 );
	phoneme_list[N_PHONEME_LIST].ph = nullptr; // recognize end of phoneme_list array
	phoneme_list[N_PHONEME_LIST].sourceix = 1;

	new_sentence = ( s.terminator & CLAUSE_TYPE_SENTENCE ) != 0; // the next clause starts a new sentence if this one ended one

	if ( voice_change != nullptr ) {
		*voice_change = nullptr; // (a voice change ends a clause only through embedded commands, which are not supported)
	}
}

// Reads the clause's text and sets up the translation of it: the pause and sentence bookkeeping, and the translators' per-clause state.
void Engine::BeginClause( Translator* tr, ClauseScan& s, int* tone_out, int* terminator_out ) {
	int tone;

	pre_pause = 0;
	any_stressed_words = false;

	if ( ( clause_start_char = count_characters ) < 0 ) {
		clause_start_char = 0;
	}

	for ( int ix = 0; ix < N_TR_SOURCE; ix++ ) {
		s.charix[ix] = 0;
	}
	s.terminator = ReadClause( tr, source, s.charix, &s.charix_top, N_TR_SOURCE, &tone, voice_change_name );

	if ( terminator_out != nullptr ) {
		*terminator_out = s.terminator;
	}

	if ( tone_out != nullptr ) {
		if ( tone == 0 ) {
			*tone_out = ( s.terminator & CLAUSE_INTONATION_TYPE ) >> 12; // tone type not overridden in ReadClause, use default
		} else {
			*tone_out = tone; // override tone type
		}
	}

	s.charix[s.charix_top + 1] = 0;
	s.charix[s.charix_top + 2] = 0x7fff;
	s.charix[s.charix_top + 3] = 0;

	s.clause_pause = ( s.terminator & CLAUSE_PAUSE ) * 10; // mS
	if ( s.terminator & CLAUSE_PAUSE_LONG ) {
		s.clause_pause = s.clause_pause * 32; // pause value is *320mS not *10mS
	}

	const char* p = source;
	while ( *p != 0 && isspace2( *p ) ) {
		p++;
	}
	if ( *p == 0 ) {
		// No characters except spaces. This is not a sentence.
		// Don't add this pause, just make up the previous pause to this value;
		s.clause_pause -= max_clause_pause;
		if ( s.clause_pause < 0 ) {
			s.clause_pause = 0;
		}

		if ( new_sentence ) {
			s.terminator |= CLAUSE_TYPE_SENTENCE; // carry forward an end-of-sentence indicator
		}
		max_clause_pause += s.clause_pause;
		s.new_sentence2 = false;
	} else {
		max_clause_pause = s.clause_pause;
		s.new_sentence2 = new_sentence;
	}
	tr->clause_terminator = s.terminator;

	if ( s.new_sentence2 ) {
		count_sentences++;
	}
	memset( &ph_list2[0], 0, sizeof( ph_list2[0] ) );
	ph_list2[0].phcode = phonPAUSE_SHORT;

	n_ph_list2 = 1;
	tr->prev_last_stress = 0;
	tr->prepause_timeout = 0;
	tr->expect_verb = 0;
	tr->expect_noun = 0;
	tr->expect_past = 0;
	tr->expect_verb_s = 0;
	tr->phonemes_repeat_count = 0;
	tr->end_stressed_vowel = 0;
	tr->prev_dict_flags[0] = 0;
	tr->prev_dict_flags[1] = 0;

	s.sbuf[0] = 0;
	s.sbuf[1] = ' ';
	s.sbuf[2] = ' ';
	s.ix = 3;

	s.words[0].start = s.ix;
	s.words[0].flags = 0;
	s.words[0].length = CalcWordLength( s.source_index, s.charix_top, s.charix, s.words, 0 );
}

// Copies the clause's text to s.sbuf one character at a time, normalised (quotes, hyphens, dots, camel case and so on), each word followed
// by a space, and notes the words in s.words with their flags.
void Engine::ScanWords( Translator* tr, ClauseScan& s ) {
	while ( !s.finished && ( s.ix < (int)sizeof( s.sbuf ) - 1 ) ) {
		ReadScanChar( tr, s );

		if ( s.phoneme_mode ) {
			s.all_upper_case = FLAG_PHONEMES;

			if ( ( s.c == ']' ) && ( s.next_in == ']' ) ) {
				s.phoneme_mode = false;
				s.source_index++;
				s.c = ' ';
			}
		} else if ( !ScanTextChar( tr, s ) ) {
			continue;
		}

		if ( IsSpace( s.c ) ) {
			if ( !EndWord( tr, s ) ) {
				continue; // multiple spaces
			}
		} else if ( s.ix < ( N_TR_SOURCE - 4 ) ) {
			s.ix += utf8_out( s.c, &s.sbuf[s.ix] );
		}
		if ( s.pre_pause_add > pre_pause ) {
			pre_pause = s.pre_pause_add;
		}
		s.pre_pause_add = 0;
	}
}

// Reads the next character into s.c, with the characters before and after it in the input and the output.
void Engine::ReadScanChar( Translator* tr, ClauseScan& s ) {
	s.prev_out2 = s.prev_out;
	utf8_in2( &s.prev_out, &s.sbuf[s.ix - 1], 1 );

	if ( tr->langopts.tone_numbers && IsDigit09( s.prev_out ) && IsAlpha( s.prev_out2 ) ) {
		// tone numbers can be part of a word, consider them as alphabetic
		s.prev_out = 'a';
	}

	if ( s.prev_in_save != 0 ) {
		s.prev_in = s.prev_in_save;
		s.prev_in_save = 0;
	} else if ( s.source_index > 0 ) {
		utf8_in2( &s.prev_in, &source[s.source_index - 1], 1 );
	}

	s.prev_source_index = s.source_index;

	if ( s.char_inserted ) {
		s.c = s.char_inserted;
		s.char_inserted = 0;
	} else {
		s.source_index += utf8_in( &s.cc, &source[s.source_index] );
		s.c = s.cc;
	}

	if ( s.c == 0 ) {
		s.finished = true;
		s.c = ' ';
		s.next_in = ' ';
		s.next_in_nbytes = 0;
	} else {
		s.next_in_nbytes = utf8_in( &s.next_in, &source[s.source_index] );
	}
}

// One character of ordinary text (not phoneme mnemonics): quote and mark normalisation, the language's character translations, and what
// the character means for the word being built. False if the character is dropped.
bool Engine::ScanTextChar( Translator* tr, ClauseScan& s ) {
	if ( ( s.c == 0x92 ) || ( s.c == 0xb4 ) || ( s.c == 0x2019 ) || ( s.c == 0x2032 ) ) {
		s.c = '\''; // 'microsoft' quote or sexed closing single quote, or prime - possibly used as apostrophe
	}

	if ( ( ( s.c == 0x2018 ) || ( s.c == '?' ) ) && IsAlpha( s.prev_out ) && IsAlpha( s.next_in ) ) {
		// ? between two letters may be a smart-quote replaced by ?
		s.c = '\'';
	}

	if ( s.c == CHAR_EMPHASIS ) {
		// this character is a marker that the previous word is the focus of the clause
		s.c = ' ';
		s.word_flags |= FLAG_FOCUS;
	}

	if ( s.c == CHAR_COMMA_BREAK ) {
		s.c = ' ';
		s.word_flags |= FLAG_COMMA_AFTER;
	}
	// language specific character translations
	s.c = TranslateChar( tr, &source[s.source_index], s.prev_in, s.c, s.next_in, &s.char_inserted, &s.word_flags );
	if ( s.c == 8 ) {
		return false; // ignore this character
	}

	if ( s.char_inserted ) {
		s.next_in = s.char_inserted;
	}

	// allow certain punctuation within a word (usually only apostrophe)
	if ( !IsAlpha( s.c ) && !IsSpace( s.c ) && ( wcschr( tr->punct_within_word, s.c ) == 0 ) ) {
		if ( IsAlpha( s.prev_out ) ) {
			if ( tr->langopts.tone_numbers && IsDigit09( s.c ) && !IsDigit09( s.next_in ) ) {
				// allow a tone number as part of the word
			} else {
				s.c = ' '; // ensure we have an end-of-word terminator
				s.space_inserted = true;
			}
		}
	}

	if ( chars::is_digit( s.prev_out ) ) {
		if ( !chars::is_digit( s.c ) && ( s.c != '.' ) && ( s.c != ',' ) && ( s.c != ' ' ) ) {
			s.c = ' '; // terminate digit string with a space
			s.space_inserted = true;
		}
	} else { // Prev output is not digit
		if ( s.prev_in == ',' ) {
			// Workaround for several consecutive commas — replace current character with space
			if ( s.c == ',' ) {
				s.c = ' ';
			}
		} else {
			s.decimal_sep_count = false;
		}
	}

	if ( s.c == '[' ) {
		if ( s.next_in == '\002' ) {
			//  "[\002" is used internally to start phoneme mode
			s.phoneme_mode = true;
			s.source_index++;
			return false;
		}
	}

	if ( IsAlpha( s.c ) ) {
		ScanLetter( tr, s );
	} else if ( s.c == '-' ) {
		ScanHyphen( tr, s );
	} else if ( s.c == '.' ) {
		ScanDot( tr, s );
	} else if ( s.c == '\'' ) {
		ScanApostrophe( tr, s );
	} else if ( lookupwchar( breaks, s.c ) != 0 ) {
		s.c = ' '; // various characters to treat as space
	} else if ( chars::is_digit( s.c ) ) {
		ScanDigit( tr, s );
	}
	return true;
}

// A letter: where words start, capital letters (camel case, plurals of abbreviations), and mixed scripts.
void Engine::ScanLetter( Translator* tr, ClauseScan& s ) {
	s.alpha_count++;
	if ( !IsAlpha( s.prev_out ) || ( tr->langopts.ideographs && ( ( s.c > 0x3040 ) || ( s.prev_out > 0x3040 ) ) ) ) {
		if ( wcschr( tr->punct_within_word, s.prev_out ) == 0 ) {
			s.letter_count = 0; // don't reset count for an apostrophy within a word
		}

		if ( ( s.prev_out != ' ' ) && ( wcschr( tr->punct_within_word, s.prev_out ) == 0 ) ) {
			// start of word, insert space if not one there already
			s.c = ' ';
			s.space_inserted = true;

			if ( !IsBracket( s.prev_out ) ) { // ?? perhaps only set FLAG_NOSPACE for . - /  (hyphenated words, URLs, etc)
				s.next_word_flags |= FLAG_NOSPACE;
			}
		} else {
			if ( chars::is_upper( s.c ) ) {
				s.word_flags |= FLAG_FIRST_UPPER;
			}

			if ( ( s.prev_out == ' ' ) && chars::is_digit( s.sbuf[s.ix - 2] ) && !chars::is_digit( s.prev_in ) ) {
				// word, following a number, but with a space between
				// Add an extra space, to distinguish "2 a" from "2a"
				s.sbuf[s.ix++] = ' ';
				s.words[s.word_count].start++;
			}
		}
	}

	if ( s.c != ' ' ) {
		s.letter_count++;

		if ( tr->letter_bits_offset > 0 ) {
			if ( ( ( s.c < 0x250 ) && ( s.prev_out >= tr->letter_bits_offset ) ) ||
				 ( ( s.c >= tr->letter_bits_offset ) && ( s.letter_count > 1 ) && ( s.prev_out < 0x250 ) ) ) {
				// Don't mix native and Latin characters in the same word
				// Break into separate words
				if ( IsAlpha( s.prev_out ) ) {
					s.c = ' ';
					s.space_inserted = true;
					s.word_flags |= FLAG_HYPHEN_AFTER;
					s.next_word_flags |= FLAG_HYPHEN;
				}
			}
		}
	}

	if ( chars::is_upper( s.c ) ) {
		s.c = towlower2( s.c, tr );

		if ( tr->langopts.param[LOPT_CAPS_IN_WORD] ) {
			if ( s.syllable_marked == false ) {
				s.char_inserted = s.c;
				s.c = 0x2c8; // stress marker
				s.syllable_marked = true;
			}
		} else {
			if ( chars::is_lower( s.prev_in ) ) {
				// lower case followed by upper case, possibly CamelCase
				if ( ( s.prev_out != ' ' ) && UpperCaseInWord( tr, &s.sbuf[s.ix], s.c ) == 0 ) { // start a new word
					s.c = ' ';
					s.space_inserted = true;
					s.prev_in_save = s.c;
				}
			} else if ( ( s.c != ' ' ) && chars::is_upper( s.prev_in ) && chars::is_lower( s.next_in ) ) {
				int next2_in;
				utf8_in( &next2_in, &source[s.source_index + s.next_in_nbytes] );

				if ( ( tr->translator_name == L( 'n', 'l' ) ) && ( s.letter_count == 2 ) && ( s.c == 'j' ) && ( s.prev_in == 'I' ) ) {
					// Dutch words may capitalise initial IJ, don't split
				} else if ( ( s.prev_out != ' ' ) && IsAlpha( next2_in ) ) {
					// changing from upper to lower case, start new word at the last uppercase, if 3 or more letters
					s.c = ' ';
					s.space_inserted = true;
					s.prev_in_save = s.c;
					s.next_word_flags |= FLAG_NOSPACE;
				}
			}
		}
	} else {
		if ( ( s.all_upper_case ) && ( s.letter_count > 2 ) ) {
			// Flag as plural only English
			if ( tr->translator_name == L( 'e', 'n' ) && ( s.c == 's' ) && ( s.next_in == ' ' ) ) {
				s.c = ' ';
				s.all_upper_case |= FLAG_HAS_PLURAL;

				if ( s.sbuf[s.ix - 1] == '\'' ) {
					s.sbuf[s.ix - 1] = ' ';
				}
			} else {
				s.all_upper_case = 0; // current word contains lower case letters, not "'s"
			}
		} else {
			s.all_upper_case = 0;
		}
	}
}

// A hyphen: a hyphen within a word, a dash, or a pause.
void Engine::ScanHyphen( Translator* tr, ClauseScan& s ) {
	if ( !IsSpace( s.prev_in ) && IsAlpha( s.next_in ) ) {
		if ( s.prev_out != ' ' ) {
			// previous 'word' not yet ended (not alpha or numeric), start new word now.
			s.c = ' ';
			s.space_inserted = true;
		} else {
			// '-' between two letters is a hyphen, treat as a space
			s.word_flags |= FLAG_HYPHEN;
			if ( s.word_count > 0 ) {
				s.words[s.word_count - 1].flags |= FLAG_HYPHEN_AFTER;
			}
			s.c = ' ';
		}
	} else if ( ( s.prev_in == ' ' ) && ( s.next_in == ' ' ) ) {
		// ' - ' dash between two spaces, treat as pause
		s.c = ' ';
		s.pre_pause_add = 4;
	} else if ( s.next_in == '-' ) {
		// double hyphen, treat as pause
		s.source_index++;
		s.c = ' ';
		s.pre_pause_add = 4;
	} else if ( ( s.prev_out == ' ' ) && IsAlpha( s.prev_out2 ) && !IsAlpha( s.prev_in ) ) {
		// insert extra space between a word + space + hyphen, to distinguish 'a -2' from 'a-2'
		s.sbuf[s.ix++] = ' ';
		s.words[s.word_count].start++;
	}
}

// A dot: part of an abbreviation, or removed.
void Engine::ScanDot( Translator* tr, ClauseScan& s ) {
	if ( s.prev_out == '.' ) {
		// multiple dots, separate by spaces. Note >3 dots has been replaced by elipsis
		s.c = ' ';
		s.space_inserted = true;
	} else if ( ( s.word_count > 0 ) && !( s.words[s.word_count - 1].flags & FLAG_NOSPACE ) && IsAlpha( s.prev_in ) ) {
		// dot after a word, with space following, probably an abbreviation
		s.words[s.word_count - 1].flags |= FLAG_HAS_DOT;

		if ( IsSpace( s.next_in ) || ( s.next_in == '-' ) ) {
			s.c = ' '; // remove the dot if it's followed by a space or hyphen, so that it's not pronounced
		}
	}
}

// An apostrophe or single quote: part of the word, or a quote that ends it with a pause.
void Engine::ScanApostrophe( Translator* tr, ClauseScan& s ) {
	if ( ( ( s.prev_in == '.' && s.next_in == 's' ) || chars::is_alnum( s.prev_in ) ) && IsAlpha( s.next_in ) ) {
		// between two letters, or in an abbreviation (eg. u.s.a.'s). Consider the apostrophe as part of the word
		s.single_quoted = false;
	} else if ( ( tr->langopts.param[LOPT_APOSTROPHE] & 1 ) && IsAlpha( s.next_in ) ) {
		s.single_quoted = false; // apostrophe at start of word is part of the word
	} else if ( ( tr->langopts.param[LOPT_APOSTROPHE] & 2 ) && IsAlpha( s.prev_in ) ) {
		s.single_quoted = false; // apostrophe at end of word is part of the word
	} else if ( ( wcschr( tr->char_plus_apostrophe, s.prev_in ) != 0 ) && ( s.prev_out2 == ' ' ) ) {
		// consider single character plus apostrophe as a word
		s.single_quoted = false;
		if ( s.next_in == ' ' ) {
			s.source_index++; // skip following space
		}
	} else {
		if ( ( s.prev_out == 's' ) && ( s.single_quoted == false ) ) {
			// looks like apostrophe after an 's'
			s.c = ' ';
		} else {
			s.single_quoted = IsSpace( s.prev_out );
			s.pre_pause_add = 4; // single quote
			s.c = ' ';
		}
	}
}

// A digit: where a number starts and ends, and its decimal separator.
void Engine::ScanDigit( Translator* tr, ClauseScan& s ) {
	if ( tr->langopts.tone_numbers && IsAlpha( s.prev_out ) && !IsDigit( s.next_in ) ) {
	} else if ( ( s.prev_out != ' ' ) && !chars::is_digit( s.prev_out ) ) {
		if ( ( s.prev_out != tr->langopts.decimal_sep ) || ( ( s.decimal_sep_count == true ) && ( tr->langopts.decimal_sep == ',' ) ) ) {
			s.c = ' ';
			s.space_inserted = true;
		} else {
			s.decimal_sep_count = true;
		}
	} else if ( ( s.prev_out == ' ' ) && IsAlpha( s.prev_out2 ) && !IsAlpha( s.prev_in ) ) {
		// insert extra space between a word and a number, to distinguish 'a 2' from 'a2'
		s.sbuf[s.ix++] = ' ';
		s.words[s.word_count].start++;
	}
}

// A space ends the current word: it is recorded in s.words. False for a space that follows a space (nothing happens).
bool Engine::EndWord( Translator* tr, ClauseScan& s ) {
	if ( s.prev_out == ' ' ) {
		s.word_flags |= FLAG_MULTIPLE_SPACES;
		return false;
	}

	if ( ( s.cc == 0x09 ) || ( s.cc == 0x0a ) ) {
		s.next_word_flags |= FLAG_MULTIPLE_SPACES; // tab or newline, not a simple space
	}

	if ( s.space_inserted ) {
		// count the number of characters since the start of the word
		int count = 0;
		int k = s.source_index - 1;
		while ( ( k >= s.source_index_word ) && ( s.charix[k] != 0 ) ) {
			if ( s.charix[k] > 0 ) { // don't count initial bytes of multi-byte character
				count++;
			}
			k--;
		}
		s.words[s.word_count].length = count;
	}

	s.source_index_word = s.source_index;

	// end of 'word'
	s.sbuf[s.ix++] = ' ';

	if ( ( s.word_count < N_CLAUSE_WORDS - 1 ) && ( s.ix > s.words[s.word_count].start ) ) {
		if ( s.alpha_count == 0 ) {
			s.all_upper_case &= ~FLAG_ALL_UPPER;
		}
		s.words[s.word_count].pre_pause = pre_pause;
		s.words[s.word_count].flags |= ( s.all_upper_case | s.word_flags );

		if ( pre_pause > 0 ) {
			// insert an extra space before the word, to prevent influence from previous word across the pause
			int j;
			for ( j = s.ix; j > s.words[s.word_count].start; j-- ) {
				s.sbuf[j] = s.sbuf[j - 1];
			}
			s.sbuf[j] = ' ';
			s.words[s.word_count].start++;
			s.ix++;
		}

		s.word_count++;
		s.words[s.word_count].start = s.ix;
		s.words[s.word_count].flags = 0;

		s.words[s.word_count].length = CalcWordLength( s.source_index, s.charix_top, s.charix, s.words, s.word_count );

		s.word_flags = s.next_word_flags;
		s.next_word_flags = 0;
		pre_pause = 0;
		s.all_upper_case = FLAG_ALL_UPPER;
		s.alpha_count = 0;
		s.syllable_marked = false;
	}

	if ( s.space_inserted ) {
		s.source_index = s.prev_source_index; // rewind to the previous character
		s.char_inserted = 0;
		s.space_inserted = false;
	}
	return true;
}

// The scan is done: terminate the text, and mark the first and last words and the last word's dot.
void Engine::FinishWordList( Translator* tr, ClauseScan& s ) {
	tr->clause_end = &s.sbuf[s.ix - 1];
	s.sbuf[s.ix] = 0;
	s.words[0].pre_pause = 0; // don't add extra pause at beginning of clause
	s.words[s.word_count].pre_pause = 8;
	if ( s.word_count > 0 ) {
		int last = s.word_count - 1;
		while ( ( last > 0 ) && ( IsBracket( s.sbuf[s.words[last].start] ) ) ) {
			last--; // the last word is a bracket, mark the previous word as last
		}
		s.words[last].flags |= FLAG_LAST_WORD;

		// FLAG_NOSPACE check to avoid recognizing  .mr  -mr
		if ( ( s.terminator & CLAUSE_DOT_AFTER_LAST_WORD ) && !( s.words[s.word_count - 1].flags & FLAG_NOSPACE ) ) {
			s.words[s.word_count - 1].flags |= FLAG_HAS_DOT;
		}
	}
	s.words[0].flags |= FLAG_FIRST_WORD;
}

// Translates each word of the clause to phonemes (TranslateWord2), splitting long numbers into groups of digits first.
void Engine::TranslateWords( Translator* tr, ClauseScan& s, int* tone_out ) {
	// Each TranslateWord2 may require up to 7 phonemes
	// and after this loop we require 2 phonemes
	for ( int ix = 0; ix < s.word_count && ( n_ph_list2 < N_PHONEME_LIST - 7 - 2 ); ix++ ) {
		int dict_flags = 0; // returned from dictionary lookup

		count_words++;
		current_alphabet = nullptr;

		// digits should have been converted to Latin alphabet ('0' to '9')
		char* word = &s.sbuf[s.words[ix].start];

		if ( chars::is_digit( word[0] ) && ( tr->langopts.break_numbers != BREAK_THOUSANDS ) ) {
			RemoveThousandsSeparators( tr, s, ix, word );
		}

		int n_digits = 0;
		while ( chars::is_digit( word[n_digits] ) ) { // count consecutive digits
			n_digits++;
		}

		if ( n_digits > 4 && n_digits <= 32 ) {
			dict_flags = TranslateLongNumber( tr, s, ix, word, n_digits );
		} else {
			dict_flags = TranslatePlainWord( tr, s, ix, word, tone_out );
		}

		if ( dict_flags & FLAG_SKIPWORDS ) {
			// dictionary indicates skip next word(s)
			while ( dictionary_skipwords > 0 ) {
				s.words[ix + dictionary_skipwords].flags |= FLAG_DELETE_WORD;
				dictionary_skipwords--;
			}
		}
	}
}

// Languages with 100000 numbers: remove thousands separators from the number at `word`, so that they can be inserted again later. Words
// the number spanned are skipped (ix moves past them).
void Engine::RemoveThousandsSeparators( Translator* tr, ClauseScan& s, int& ix, char* word ) {
	char number_buf[150];
	char* pw = word;
	char* pn = number_buf;
	while ( pn < &number_buf[sizeof( number_buf ) - 20] ) {
		if ( chars::is_digit( *pw ) ) {
			*pn++ = *pw++;
		} else if ( ( *pw == tr->langopts.thousands_sep ) && ( pw[1] == ' ' ) && chars::is_digit( pw[2] ) && ( pw[3] != ' ' ) &&
					( pw[4] != ' ' ) ) { // don't allow only 1 or 2 digits in the final part
			pw += 2;
			ix++; // skip "word"
		} else {
			memset( word, ' ', pw - word );
			memcpy( word, number_buf, pn - number_buf );
			break;
		}
	}
}

// A number of more than 4 digits: breaks it into groups of digits (with the language's thousands separator) and translates each group as a
// word. Returns the dictionary flags of the last.
int Engine::TranslateLongNumber( Translator* tr, ClauseScan& s, int ix, char* word, int n_digits ) {
	char number_buf[150];
	WORD_TAB num_wtab[50]; // copy of 'words', when splitting numbers into parts
	int dict_flags = 0;
	int c;
	int nw = 0;
	char* pw = word;

	number_buf[0] = ' ';
	number_buf[1] = ' ';
	number_buf[2] = ' ';
	char* pn = &number_buf[3];
	int nx = n_digits;

	if ( ( n_digits > tr->langopts.max_digits ) || ( word[0] == '0' ) ) {
		s.words[ix].flags |= FLAG_INDIVIDUAL_DIGITS;
	}

	while ( pn < &number_buf[sizeof( number_buf ) - 20] ) {
		if ( !IsDigit09( c = *pw++ ) && ( c != tr->langopts.decimal_sep ) ) {
			break;
		}

		*pn++ = c;
		nx--;
		if ( ( nx > 0 ) && ( tr->langopts.break_numbers & ( 1U << nx ) ) ) {
			memcpy( &num_wtab[nw++], &s.words[ix], sizeof( WORD_TAB ) ); // copy the 'words' entry for each word of numbers

			if ( tr->langopts.thousands_sep != ' ' ) {
				*pn++ = tr->langopts.thousands_sep;
			}
			*pn++ = ' ';

			if ( ( s.words[ix].flags & FLAG_INDIVIDUAL_DIGITS ) == 0 ) {
				if ( tr->langopts.break_numbers & ( 1 << ( nx - 1 ) ) ) {
					// the next group only has 1 digits, make it three
					*pn++ = '0';
					*pn++ = '0';
				}
				if ( tr->langopts.break_numbers & ( 1 << ( nx - 2 ) ) ) {
					// the next group only has 2 digits (eg. Indian languages), make it three
					*pn++ = '0';
				}
			}
		}
	}
	pw--;
	memcpy( &num_wtab[nw], &s.words[ix], sizeof( WORD_TAB ) * 2 ); // the original number word, and the word after it

	for ( int j = 1; j <= nw; j++ ) {
		num_wtab[j].flags &= ~FLAG_MULTIPLE_SPACES; // don't use this flag for subsequent parts when splitting a number
	}

	// include the next few characters, in case there are an ordinal indicator or other suffix
	memcpy( pn, pw, 16 );
	pn[16] = 0;
	nw = 0;

	for ( pw = &number_buf[3]; pw < pn; ) {
		// keep wflags for each part, for FLAG_HYPHEN_AFTER
		dict_flags = TranslateWord2( tr, pw, &num_wtab[nw++], s.words[ix].pre_pause );
		while ( *pw++ != ' ' ) {}
		s.words[ix].pre_pause = 0;
	}
	return dict_flags;
}

// An ordinary word. Returns its dictionary flags. A word that asks to be spelled is redone letter by letter; one that may be an
// abbreviation at the end of the clause shortens the clause's pause.
int Engine::TranslatePlainWord( Translator* tr, ClauseScan& s, int ix, char* word, int* tone_out ) {
	pre_pause = 0;

	const int dict_flags = TranslateWord2( tr, word, &s.words[ix], s.words[ix].pre_pause );

	if ( pre_pause > s.words[ix + 1].pre_pause ) {
		s.words[ix + 1].pre_pause = pre_pause;
		pre_pause = 0;
	}

	if ( dict_flags & FLAG_SPELLWORD ) {
		// redo the word, speaking single letters
		char number_buf[150];
		for ( char* pw = word; *pw != ' '; ) {
			memset( number_buf, 0, sizeof( number_buf ) );
			memset( number_buf, ' ', 9 );
			int c_temp;
			const int nx = utf8_in( &c_temp, pw );
			memcpy( &number_buf[2], pw, nx );
			TranslateWord2( tr, &number_buf[2], &s.words[ix], 0 );
			pw += nx;
		}
	}

	if ( ( dict_flags & ( FLAG_ALLOW_DOT | FLAG_NEEDS_DOT ) ) && ( ix == s.word_count - 1 - dictionary_skipwords ) &&
		 ( s.terminator & CLAUSE_DOT_AFTER_LAST_WORD ) ) {
		// probably an abbreviation such as Mr. or B. rather than end of sentence
		s.clause_pause = 10;
		if ( tone_out != nullptr ) {
			*tone_out = 4;
		}
	}
	return dict_flags;
}

void Engine::TranslateClause( Translator* tr, int* tone_out, char** voice_change ) {
	TranslateClauseWithTerminator( tr, tone_out, voice_change, nullptr );
}

int Engine::CalcWordLength( int source_index, int charix_top, short int* charix, WORD_TAB* words, int word_count ) {
	int j;
	int k;

	for ( j = source_index; j < charix_top && charix[j] <= 0; j++ )
		; // skip blanks
	words[word_count].sourceix = charix[j];
	k = 0;
	while ( charix[j] != 0 ) {
		// count the number of characters (excluding multibyte continuation bytes)
		if ( charix[j++] != -1 ) {
			k++;
		}
	}
	return k;
}

void Engine::CombineFlag( Translator* tr, WORD_TAB* wtab, char* word, int* flags, unsigned char* p, char* word_phonemes ) {
	// combine a preposition with the following word


	int sylimit; // max. number of syllables in a word to be combined with a preceding preposition
	sylimit = tr->langopts.param[LOPT_COMBINE_WORDS];


	char* p2;
	p2 = word;
	while ( *p2 != ' ' ) {
		p2++;
	}

	bool ok = true;
	int c_word2;

	utf8_in( &c_word2, p2 + 1 ); // first character of the next word;

	if ( !chars::is_alpha( c_word2 ) ) {
		ok = false;
	}

	int flags2[2];
	flags2[0] = 0;


	if ( ok ) {
		char ph_buf[N_WORD_PHONEMES];
		strcpy( ph_buf, word_phonemes );

		flags2[0] = TranslateWord( tr, p2 + 1, wtab + 1, nullptr );
		if ( ( flags2[0] & FLAG_WAS_UNPRONOUNCABLE ) || ( word_phonemes[0] == phonSWITCH ) ) {
			ok = false;
		}

		if ( ( sylimit & 0x100 ) && ( ( flags2[0] & FLAG_ALT_TRANS ) == 0 ) ) {
			// only if the second word has $alt attribute
			ok = false;
		}

		if ( ( sylimit & 0x200 ) && ( ( wtab + 1 )->flags & FLAG_LAST_WORD ) ) {
			// not if the next word is end-of-sentence
			ok = false;
		}

		if ( ok == false ) {
			strcpy( word_phonemes, ph_buf );
		}
	}

	if ( ok ) {
		*p2 = '-';												   // replace next space by hyphen
		wtab[0].flags &= ~FLAG_ALL_UPPER;						   // prevent it being considered an abbreviation
		*flags = TranslateWord( translator, word, wtab, nullptr ); // translate the combined word
		if ( ( sylimit > 0 ) && ( CountSyllables( p ) > ( sylimit & 0x1f ) ) ) {
			// revert to separate words
			*p2 = ' ';
			*flags = TranslateWord( translator, word, wtab, nullptr );
		} else {
			if ( *flags == 0 ) {
				*flags = flags2[0]; // no flags for the combined word, so use flags from the second word eg. lang-hu "nem december 7-e"
			}
			*flags |= FLAG_SKIPWORDS;
			dictionary_skipwords = 1;
		}
	}
}

void Engine::SwitchLanguage( char* word, char* word_phonemes ) {
	char lang_name[12];
	int ix;

	word += 3;

	for ( ix = 0;; ) {
		int c1;
		c1 = *word++;
		if ( ( c1 == ' ' ) || ( c1 == 0 ) ) {
			break;
		}
		lang_name[ix++] = tolower( c1 );
	}
	lang_name[ix] = 0;

	if ( ( ix = LookupPhonemeTable( lang_name ) ) > 0 ) {
		SelectPhonemeTable( ix );
		word_phonemes[0] = phonSWITCH;
		word_phonemes[1] = ix;
		word_phonemes[2] = 0;
	}
}

} // namespace propan2ol
