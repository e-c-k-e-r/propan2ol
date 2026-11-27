
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

// Part of the Engine: one word (espeak-ng's translateword.c): the dictionary lookup and the rules (TranslateWord3), spelling out unpronounceable
// words and abbreviations letter by letter, and the language-specific word attributes.

#include "engine.h"

namespace propan2ol {

// One word to phonemes (TranslateWord2's lookup): the dictionary list first, then the rules with prefixes and suffixes removed, then stress.
// `word_start` is the word, terminated by a space (0x20); the phonemes go to `word_phonemes_arg`. Returns the dictionary flags.
int Engine::TranslateWord3( Translator* tr, char* word_start, WORD_TAB* wtab, char* word_out, bool* any_stressed_words_arg,
							ALPHABET* current_alphabet_arg, char word_phonemes_arg[], size_t size_word_phonemes ) {
	WordParse w;
	w.word_start = word_start;
	w.wtab = wtab;
	w.word_out = word_out;
	w.any_stressed_words_arg = any_stressed_words_arg;
	w.current_alphabet_arg = current_alphabet_arg;
	w.word_phonemes_arg = word_phonemes_arg;
	w.size_word_phonemes = size_word_phonemes;

	if ( std::optional<int> result = LookUpWord( tr, w ) ) {
		return *result;
	}

	if ( w.spell_word > 0 ) {
		return SpellWord( tr, w );
	}
	if ( !w.found ) {
		// the word's pronunciation is not given in the dictionary list, although dictionary_flags may have been set there
		if ( std::optional<int> result = TranslateUnlistedWord( tr, w ) ) {
			return *result;
		}
	}
	return FinishWord( tr, w );
}

// The first lookup of the word in the dictionary list, and the decisions that follow from it (a text replacement, a language switch,
// digits, a roman numeral, an abbreviation to spell out). Returns the final result if it settles the word, else nothing.
std::optional<int> Engine::LookUpWord( Translator* tr, WordParse& w ) {
	int ix;
	if ( w.wtab == nullptr ) {
		memset( w.wtab_null, 0, sizeof( w.wtab_null ) );
		w.wtab = w.wtab_null;
	}
	w.wflags = w.wtab->flags;

	w.dictionary_flags[0] = 0;
	w.dictionary_flags[1] = 0;
	w.dictionary_flags2[0] = 0;
	w.dictionary_flags2[1] = 0;
	dictionary_skipwords = 0;

	w.phonemes[0] = 0;
	w.unpron_phonemes[0] = 0;
	w.prefix_phonemes[0] = 0;
	w.end_phonemes[0] = 0;

	if ( tr->data_dictlist == nullptr ) {
		// dictionary is not loaded
		w.word_phonemes_arg[0] = 0;
		return 0;
	}

	// count the length of the word
	w.word1 = w.word_start;
	if ( *w.word1 == ' ' ) {
		w.word1++; // possibly a dot was replaced by space:  $dot
	}
	w.wordx = w.word1;

	utf8_in( &w.first_char, w.wordx );
	w.word_length = 0;
	while ( ( *w.wordx != 0 ) && ( *w.wordx != ' ' ) ) {
		w.wordx += utf8_in( &w.last_char, w.wordx );
		w.word_length++;
	}

	w.word_copy_length = w.wordx - w.word_start;
	if ( w.word_copy_length >= N_WORD_BYTES ) {
		w.word_copy_length = N_WORD_BYTES - 1;
	}
	memcpy( w.word_copy2, w.word_start, w.word_copy_length );

	w.spell_word = 0;

	if ( ( w.word_length == 1 ) && ( w.wflags & FLAG_TRANSLATOR2 ) ) {
		// retranslating a 1-character word using a different language, say its name
		utf8_in( &w.c_temp, w.wordx + 1 ); // the next character
		if ( !IsAlpha( w.c_temp ) || ( AlphabetFromChar( w.last_char ) != AlphabetFromChar( w.c_temp ) ) ) {
			w.spell_word = 1;
		}
	}


	// try an initial lookup in the dictionary list, we may find a pronunciation specified, or
	// we may just find some flags
	if ( !w.found ) {
		w.found = LookupDictList( tr, &w.word1, w.phonemes, w.dictionary_flags, FLAG_ALLOW_TEXTMODE, w.wtab ); // the original word
	}

	if ( ( w.dictionary_flags[0] & ( FLAG_ALLOW_DOT | FLAG_NEEDS_DOT ) ) && ( w.wordx[1] == '.' ) ) {
		w.wordx[1] = ' '; // remove a Dot after this word
	}

	if ( w.dictionary_flags[0] & FLAG_TEXTMODE ) {
		if ( w.word_out != nullptr ) {
			strcpy( w.word_out, w.word1 );
		}

		return w.dictionary_flags[0];
	} else if ( ( w.found == false ) && ( w.dictionary_flags[0] & FLAG_SKIPWORDS ) && !( w.dictionary_flags[0] & FLAG_ABBREV ) ) {
		// grouped words, but no translation.  Join the words with hyphens.
		w.wordx = w.word1;
		ix = 0;
		while ( ix < dictionary_skipwords ) {
			if ( *w.wordx == ' ' ) {
				*w.wordx = '-';
				ix++;
			}
			w.wordx++;
		}
	}

	if ( ( w.word_length == 1 ) && ( dictionary_skipwords == 0 ) ) {
		// is this a series of single letters separated by dots?
		if ( CheckDottedAbbrev( w.word1 ) ) {
			w.dictionary_flags[0] = 0;
			w.dictionary_flags[1] = 0;
			w.spell_word = 1;
			if ( dictionary_skipwords ) {
				w.dictionary_flags[0] = FLAG_SKIPWORDS;
			}
		}
	}

	if ( w.phonemes[0] == phonSWITCH ) {
		// change to another language in order to translate this word
		strcpy( w.word_phonemes_arg, w.phonemes );
		return 0;
	}

	if ( !w.found && ( w.dictionary_flags[0] & FLAG_ABBREV ) ) {
		// the word has $abbrev flag, but no pronunciation specified.  Speak as individual letters
		w.spell_word = 1;
	}

	if ( !w.found && chars::is_digit( w.first_char ) ) {
		Lookup( tr, "_0lang", w.word_phonemes_arg );
		if ( w.word_phonemes_arg[0] == phonSWITCH ) {
			return 0;
		}

		if ( ( tr->langopts.numbers2 & NUM2_ENGLISH_NUMERALS ) && !( w.wtab->flags & FLAG_CHAR_REPLACED ) ) {
			// for this language, speak English numerals (0-9) with the English voice
			sprintf( w.word_phonemes_arg, "%c", phonSWITCH );
			return 0;
		}

		w.found = TranslateNumber( tr, w.word1, w.phonemes, w.dictionary_flags, w.wtab, 0 );
	}

	if ( !w.found && ( ( w.wflags & FLAG_UPPERS ) != FLAG_FIRST_UPPER ) ) {
		// either all upper or all lower case

		if ( ( tr->langopts.numbers & NUM_ROMAN ) || ( ( tr->langopts.numbers & NUM_ROMAN_CAPITALS ) && ( w.wflags & FLAG_ALL_UPPER ) ) ) {
			if ( ( w.wflags & FLAG_LAST_WORD ) || !( w.wtab[1].flags & FLAG_NOSPACE ) ) {
				// don't use Roman number if this word is not separated from the next word (eg. "XLTest")
				if ( ( w.found = TranslateRoman( tr, w.word1, w.phonemes, w.wtab ) ) != 0 ) {
					w.dictionary_flags[0] |= FLAG_ABBREV; // prevent emphasis if capitals
				}
			}
		}
	}

	if ( ( w.wflags & FLAG_ALL_UPPER ) && ( w.word_length > 1 ) && chars::is_alpha( w.first_char ) ) {
		if ( !w.found && !( w.dictionary_flags[0] & FLAG_SKIPWORDS ) && ( w.word_length < 4 ) && ( tr->clause_lower_count > 3 ) &&
			 ( tr->clause_upper_count <= tr->clause_lower_count ) ) {
			// An upper case word in a lower case clause. This could be an abbreviation.
			w.spell_word = 1;
		}
	}
	return std::nullopt;
}

// The word is to be spoken as individual letters.
int Engine::SpellWord( Translator* tr, WordParse& w ) {
	// Speak as individual letters
	w.phonemes[0] = 0;

	if ( SpeakIndividualLetters( tr, w.word1, w.phonemes, w.spell_word, w.current_alphabet_arg, w.word_phonemes_arg ) == nullptr ) {
		if ( w.word_length > 1 ) {
			return FLAG_SPELLWORD; // a mixture of languages, retranslate as individual letters, separated by spaces
		}
		return 0;
	}
	strcpy( w.word_phonemes_arg, w.phonemes );
	if ( w.wflags & FLAG_TRANSLATOR2 ) {
		return 0;
	}

	addPluralSuffixes( w.wflags, tr, w.last_char, w.word_phonemes_arg );
	return w.dictionary_flags[0] & FLAG_SKIPWORDS; // for "b.c.d"
}

// The word has no entry in the dictionary list: the rules give its pronunciation, unpronounceable lead-in letters are spelled, and the
// standard prefixes and suffixes the rules recognise are removed and the stem translated again. Returns the final result if one of these
// settles the word, else nothing.
std::optional<int> Engine::TranslateUnlistedWord( Translator* tr, WordParse& w ) {
	// word's pronunciation is not given in the dictionary list, although
	// dictionary_flags may have ben set there

	int posn;
	bool non_initial = false;
	int length;

	posn = 0;
	length = 999;
	w.wordx = w.word1;

	while ( ( ( length < 3 ) && ( length > 0 ) ) || ( w.word_length > 1 && Unpronouncable( tr, w.wordx, posn ) ) ) {
		// This word looks "unpronouncable", so speak letters individually until we
		// find a remainder that we can pronounce.
		w.was_unpronouncable = FLAG_WAS_UNPRONOUNCABLE;

		if ( w.wordx[0] == '\'' ) {
			break;
		}

		if ( posn > 0 ) {
			non_initial = true;
		}

		w.wordx += TranslateLetter( tr, w.wordx, w.unpron_phonemes, non_initial, w.current_alphabet_arg );
		posn++;
		if ( w.unpron_phonemes[0] == phonSWITCH ) {
			// change to another language in order to translate this word
			strcpy( w.word_phonemes_arg, w.unpron_phonemes );
			if ( strcmp( &w.unpron_phonemes[1], ESPEAKNG_DEFAULT_VOICE ) == 0 ) {
				return FLAG_SPELLWORD; // _^_en must have been set in TranslateLetter(), not *_rules which uses only _^_
			}
			return 0;
		}

		length = 0;
		while ( w.wordx[length] != ' ' ) {
			length++;
		}
	}
	SetSpellingStress( tr, w.unpron_phonemes, 0, posn );

	// anything left ?
	if ( *w.wordx != ' ' ) {
		if ( ( w.unpron_phonemes[0] != 0 ) && ( w.wordx[0] != '\'' ) ) {
			// letters which have been spoken individually from affecting the pronunciation of the pronuncable part
			w.wordx[-1] = ' ';
		}

		// Translate the stem
		w.end_type = TranslateRules( tr, w.wordx, w.phonemes, N_WORD_PHONEMES, w.end_phonemes, w.wflags, w.dictionary_flags );

		if ( w.phonemes[0] == phonSWITCH ) {
			// change to another language in order to translate this word
			strcpy( w.word_phonemes_arg, w.phonemes );
			return 0;
		}

		if ( ( w.phonemes[0] == 0 ) && ( w.end_phonemes[0] == 0 ) ) {
			int wc;
			// characters not recognised, speak them individually
			// ?? should we say super/sub-script numbers and letters here?
			utf8_in( &wc, w.wordx );
			if ( ( w.word_length == 1 ) && ( IsAlpha( wc ) || IsSuperscript( wc ) ) ) {
				if ( ( w.wordx = SpeakIndividualLetters( tr, w.wordx, w.phonemes, w.spell_word, w.current_alphabet_arg, w.word_phonemes_arg ) ) ==
					 nullptr ) {
					return 0;
				}
				strcpy( w.word_phonemes_arg, w.phonemes );
				return 0;
			}
		}

		w.c_temp = w.wordx[-1];

		std::optional<int> result = RemovePrefixes( tr, w );
		if ( !result ) {
			result = RemoveSuffixes( tr, w );
		}
		if ( result ) {
			return result;
		}
		w.wordx[-1] = w.c_temp;
	}
	return std::nullopt;
}

// Removes the standard prefixes the rules recognised (end_type has SUFX_P), translating what is left. Returns the final result if the
// word turned out to be another language's, else nothing.
std::optional<int> Engine::RemovePrefixes( Translator* tr, WordParse& w ) {
	int ix;
	int pfix;
	int n_chars;
	int loopcount;
	bool confirm_prefix;

	w.found = false;
	confirm_prefix = true;
	for ( loopcount = 0; ( loopcount < 50 ) && ( w.end_type & SUFX_P ); loopcount++ ) {
		// Found a standard prefix, remove it and retranslate
		// loopcount guards against an endless loop
		if ( confirm_prefix && !( w.end_type & SUFX_B ) ) {
			int end2;
			char end_phonemes_stem[N_WORD_PHONEMES];

			// remove any standard suffix and confirm that the prefix is still recognised
			w.phonemes2[0] = 0;
			end2 = TranslateRules( tr, w.wordx, w.phonemes2, N_WORD_PHONEMES, end_phonemes_stem, w.wflags | FLAG_NO_PREFIX | FLAG_NO_TRACE,
								   w.dictionary_flags );
			if ( end2 ) {
				RemoveEnding( tr, w.wordx, end2, w.word_copy );
				w.end_type = TranslateRules( tr, w.wordx, w.phonemes, N_WORD_PHONEMES, w.end_phonemes, w.wflags | FLAG_NO_TRACE, w.dictionary_flags );
				memcpy( w.wordx, w.word_copy, strlen( w.word_copy ) );
				if ( ( w.end_type & SUFX_P ) == 0 ) {
					// after removing the suffix, the prefix is no longer recognised.
					// Keep the suffix, but don't use the prefix
					w.end_type = end2;
					strcpy( w.phonemes, w.phonemes2 );
					strcpy( w.end_phonemes, end_phonemes_stem );
				}
				confirm_prefix = false;
				continue;
			}
		}

		w.prefix_type = w.end_type;

		if ( w.prefix_type & SUFX_V ) {
			tr->expect_verb = 1; // use the verb form of the word
		}

		w.wordx[-1] = w.c_temp;

		if ( ( w.prefix_type & SUFX_B ) == 0 ) {
			for ( ix = ( w.prefix_type & 0xf ); ix > 0; ix-- ) { // num. of characters to remove
				w.wordx++;
				while ( ( *w.wordx & 0xc0 ) == 0x80 ) {
					w.wordx++; // for multibyte characters
				}
			}
		} else {
			pfix = 1;
			w.prefix_chars[0] = 0;
			n_chars = w.prefix_type & 0x3f;

			for ( ix = 0; ix < n_chars; ix++ ) { // num. of bytes to remove
				w.prefix_chars[pfix++] = *w.wordx++;

				if ( ( w.prefix_type & SUFX_B ) && ( ix == ( n_chars - 1 ) ) ) {
					w.prefix_chars[pfix - 1] = 0; // discard the last character of the prefix, this is the separator character
				}
			}
			w.prefix_chars[pfix] = 0;
		}
		w.c_temp = w.wordx[-1];
		w.wordx[-1] = ' ';
		confirm_prefix = true;
		w.wflags |= FLAG_PREFIX_REMOVED;

		if ( w.prefix_type & SUFX_B ) {
			// SUFX_B is used for Turkish, tr_rules contains " ' (Pb"
			// examine the prefix part
			char* wordpf;
			char prefix_phonemes2[12];

			strncpy0( prefix_phonemes2, w.end_phonemes, sizeof( prefix_phonemes2 ) );
			wordpf = &w.prefix_chars[1];
			strcpy( w.prefix_phonemes, w.phonemes );

			// look for stress marker or $abbrev
			w.found = LookupDictList( tr, &wordpf, w.phonemes, w.dictionary_flags, 0, w.wtab );
			if ( w.found ) {
				strcpy( w.prefix_phonemes, w.phonemes );
			}
			if ( w.dictionary_flags[0] & FLAG_ABBREV ) {
				w.prefix_phonemes[0] = 0;
				SpeakIndividualLetters( tr, wordpf, w.prefix_phonemes, 1, w.current_alphabet_arg, w.word_phonemes_arg );
			}
		} else {
			append( w.prefix_phonemes, w.end_phonemes );
		}
		w.end_phonemes[0] = 0;

		w.end_type = 0;
		w.found = LookupDictList( tr, &w.wordx, w.phonemes, w.dictionary_flags2, SUFX_P, w.wtab ); // without prefix
		if ( w.dictionary_flags[0] == 0 ) {
			w.dictionary_flags[0] = w.dictionary_flags2[0];
			w.dictionary_flags[1] = w.dictionary_flags2[1];
		} else {
			w.prefix_flags = 1;
		}
		if ( w.found == false ) {
			w.end_type = TranslateRules( tr, w.wordx, w.phonemes, N_WORD_PHONEMES, w.end_phonemes,
										 w.wflags & ( FLAG_HYPHEN_AFTER | FLAG_PREFIX_REMOVED ), w.dictionary_flags );

			if ( w.phonemes[0] == phonSWITCH ) {
				// change to another language in order to translate this word
				w.wordx[-1] = w.c_temp;
				strcpy( w.word_phonemes_arg, w.phonemes );
				return 0;
			}
		}
	}
	return std::nullopt;
}

// Removes the standard suffixes the rules recognised (end_type is a suffix), translating the stem and adding the suffix phonemes back.
// Returns the final result if the word turned out to be another language's, else nothing.
std::optional<int> Engine::RemoveSuffixes( Translator* tr, WordParse& w ) {
	int end_flags;
	bool more_suffixes;

	if ( ( w.end_type != 0 ) && !( w.end_type & SUFX_P ) ) {
		w.end_type1 = w.end_type;
		strcpy( w.phonemes2, w.phonemes );

		// The word has a standard ending, re-translate without this ending
		end_flags = RemoveEnding( tr, w.wordx, w.end_type, w.word_copy );
		more_suffixes = true;

		while ( more_suffixes ) {
			more_suffixes = false;
			w.phonemes[0] = 0;

			if ( w.prefix_phonemes[0] != 0 ) {
				// lookup the stem without the prefix removed
				w.wordx[-1] = w.c_temp;
				w.found = LookupDictList( tr, &w.word1, w.phonemes, w.dictionary_flags2, end_flags, w.wtab ); // include prefix, but not suffix
				w.wordx[-1] = ' ';
				if ( w.phonemes[0] == phonSWITCH ) {
					// change to another language in order to translate this word
					memcpy( w.wordx, w.word_copy, strlen( w.word_copy ) );
					strcpy( w.word_phonemes_arg, w.phonemes );
					return 0;
				}
				if ( w.dictionary_flags[0] == 0 ) {
					w.dictionary_flags[0] = w.dictionary_flags2[0];
					w.dictionary_flags[1] = w.dictionary_flags2[1];
				}
				if ( w.found ) {
					w.prefix_phonemes[0] = 0; // matched whole word, don't need prefix now
				}

				if ( ( w.found == false ) && ( w.dictionary_flags2[0] != 0 ) ) {
					w.prefix_flags = 1;
				}
			}
			if ( w.found == false ) {
				w.found = LookupDictList( tr, &w.wordx, w.phonemes, w.dictionary_flags2, end_flags, w.wtab ); // without prefix and suffix
				if ( w.phonemes[0] == phonSWITCH ) {
					// change to another language in order to translate this word
					memcpy( w.wordx, w.word_copy, strlen( w.word_copy ) );
					strcpy( w.word_phonemes_arg, w.phonemes );
					return 0;
				}

				if ( w.dictionary_flags[0] == 0 ) {
					w.dictionary_flags[0] = w.dictionary_flags2[0];
					w.dictionary_flags[1] = w.dictionary_flags2[1];
				}
			}
			if ( w.found == false ) {
				if ( w.end_type & SUFX_Q ) {
					// don't retranslate, use the original lookup result
					strcpy( w.phonemes, w.phonemes2 );
				} else {
					if ( end_flags & FLAG_SUFX ) {
						w.wflags |= FLAG_SUFFIX_REMOVED;
					}
					if ( w.end_type & SUFX_A ) {
						w.wflags |= FLAG_SUFFIX_VOWEL;
					}

					if ( w.end_type & SUFX_M ) {
						// allow more suffixes before this suffix
						strcpy( w.end_phonemes2, w.end_phonemes );
						w.end_type = TranslateRules( tr, w.wordx, w.phonemes, N_WORD_PHONEMES, w.end_phonemes, w.wflags, w.dictionary_flags );
						append( w.end_phonemes, w.end_phonemes2 ); // add the phonemes for the previous suffixes after this one

						if ( ( w.end_type != 0 ) && !( w.end_type & SUFX_P ) ) {
							// there is another suffix
							end_flags = RemoveEnding( tr, w.wordx, w.end_type, nullptr );
							more_suffixes = true;
						}
					} else {
						// don't remove any previous suffix
						TranslateRules( tr, w.wordx, w.phonemes, N_WORD_PHONEMES, nullptr, w.wflags, w.dictionary_flags );
						w.end_type = 0;
					}

					if ( w.phonemes[0] == phonSWITCH ) {
						// change to another language in order to translate this word
						strcpy( w.word_phonemes_arg, w.phonemes );
						memcpy( w.wordx, w.word_copy, strlen( w.word_copy ) );
						w.wordx[-1] = w.c_temp;
						return 0;
					}
				}
			}
		}


		if ( ( w.end_type1 & SUFX_T ) == 0 ) {
			// the default is to add the suffix and then determine the word's stress pattern
			AppendPhonemes( tr, w.phonemes, N_WORD_PHONEMES, w.end_phonemes );
			w.end_phonemes[0] = 0;
		}
		memcpy( w.wordx, w.word_copy, strlen( w.word_copy ) );
	}
	return std::nullopt;
}

// The phonemes are known: the plural ending, the stress pattern (with any prefix), and what the word's flags say about the words that follow.
int Engine::FinishWord( Translator* tr, WordParse& w ) {
	char* p;
	int ix;
	int prefix_stress;
	int add_suffix_phonemes;

	addPluralSuffixes( w.wflags, tr, w.last_char, w.word_phonemes_arg );

	// determine stress pattern for this word

	add_suffix_phonemes = 0;
	if ( w.end_phonemes[0] != 0 ) {
		add_suffix_phonemes = 2;
	}

	prefix_stress = 0;
	for ( p = w.prefix_phonemes; *p != 0; p++ ) {
		if ( ( *p == phonSTRESS_P ) || ( *p == phonSTRESS_P2 ) ) {
			prefix_stress = *p;
		}
	}
	if ( w.prefix_flags || ( prefix_stress != 0 ) ) {
		if ( ( tr->langopts.param[LOPT_PREFIXES] ) || ( w.prefix_type & SUFX_T ) ) {
			char* p;
			// German, keep a secondary stress on the stem
			SetWordStress( tr, w.phonemes, w.dictionary_flags, 3, 0 );

			// reduce all but the first primary stress
			ix = 0;
			for ( p = w.prefix_phonemes; *p != 0; p++ ) {
				if ( *p == phonSTRESS_P ) {
					if ( ix == 0 ) {
						ix = 1;
					} else {
						*p = phonSTRESS_3;
					}
				}
			}
			snprintf( w.word_phonemes_arg, w.size_word_phonemes, "%s%s%s", w.unpron_phonemes, w.prefix_phonemes, w.phonemes );

			w.word_phonemes_arg[N_WORD_PHONEMES - 1] = 0;
			SetWordStress( tr, w.word_phonemes_arg, w.dictionary_flags, -1, 0 );
		} else {
			// stress position affects the whole word, including prefix
			snprintf( w.word_phonemes_arg, w.size_word_phonemes, "%s%s%s", w.unpron_phonemes, w.prefix_phonemes, w.phonemes );
			w.word_phonemes_arg[N_WORD_PHONEMES - 1] = 0;
			SetWordStress( tr, w.word_phonemes_arg, w.dictionary_flags, -1, 0 );
		}
	} else {
		SetWordStress( tr, w.phonemes, w.dictionary_flags, -1, add_suffix_phonemes );
		snprintf( w.word_phonemes_arg, w.size_word_phonemes, "%s%s%s", w.unpron_phonemes, w.prefix_phonemes, w.phonemes );
		w.word_phonemes_arg[N_WORD_PHONEMES - 1] = 0;
	}

	if ( w.end_phonemes[0] != 0 ) {
		// a suffix had the SUFX_T option set, add the suffix after the stress pattern has been determined
		ix = strlen( w.word_phonemes_arg );
		w.end_phonemes[N_WORD_PHONEMES - 1 - ix] = 0; // ensure no buffer overflow
		strcpy( &w.word_phonemes_arg[ix], w.end_phonemes );
	}

	if ( w.wflags & FLAG_LAST_WORD ) {
		// don't use $brk pause before the last word of a sentence
		// (but allow it for emphasis, see below
		w.dictionary_flags[0] &= ~FLAG_PAUSE1;
	}

	if ( ( w.wflags & FLAG_HYPHEN ) && ( tr->langopts.stress_flags & S_HYPEN_UNSTRESS ) ) {
		ChangeWordStress( tr, w.word_phonemes_arg, 3 );
	} else if ( w.wflags & FLAG_EMPHASIZED2 ) {
		// A word is indicated in the source text as stressed
		// Give it stress level 6 (for the intonation module)
		ChangeWordStress( tr, w.word_phonemes_arg, 6 );
	} else if ( w.wtab[dictionary_skipwords].flags & FLAG_LAST_WORD ) {
		// the word has attribute to stress or unstress when at end of clause
		if ( w.dictionary_flags[0] & ( FLAG_STRESS_END | FLAG_STRESS_END2 ) ) {
			ChangeWordStress( tr, w.word_phonemes_arg, 4 );
		} else if ( ( w.dictionary_flags[0] & FLAG_UNSTRESS_END ) && ( w.any_stressed_words_arg ) ) {
			ChangeWordStress( tr, w.word_phonemes_arg, 3 );
		}
	}

	// dictionary flags for this word give a clue about which alternative pronunciations of
	// following words to use.
	if ( w.end_type1 & SUFX_F ) {
		// expect a verb form, with or without -s suffix
		tr->expect_verb = 2;
		tr->expect_verb_s = 2;
	}

	if ( w.dictionary_flags[1] & FLAG_PASTF ) {
		// expect perfect tense in next two words
		tr->expect_past = 3;
		tr->expect_verb = 0;
		tr->expect_noun = 0;
	} else if ( w.dictionary_flags[1] & FLAG_VERBF ) {
		// expect a verb in the next word
		tr->expect_verb = 2;
		tr->expect_verb_s = 0; // verb won't have -s suffix
		tr->expect_noun = 0;
	} else if ( w.dictionary_flags[1] & FLAG_VERBSF ) {
		// expect a verb, must have a -s suffix
		tr->expect_verb = 0;
		tr->expect_verb_s = 2;
		tr->expect_past = 0;
		tr->expect_noun = 0;
	} else if ( w.dictionary_flags[1] & FLAG_NOUNF ) {
		// not expecting a verb next
		tr->expect_noun = 2;
		tr->expect_verb = 0;
		tr->expect_verb_s = 0;
		tr->expect_past = 0;
	}

	if ( ( w.wordx[0] != 0 ) && ( !( w.dictionary_flags[1] & FLAG_VERB_EXT ) ) ) {
		if ( tr->expect_verb > 0 ) {
			tr->expect_verb--;
		}

		if ( tr->expect_verb_s > 0 ) {
			tr->expect_verb_s--;
		}

		if ( tr->expect_noun > 0 ) {
			tr->expect_noun--;
		}

		if ( tr->expect_past > 0 ) {
			tr->expect_past--;
		}
	}

	if ( ( w.word_length == 1 ) && ( tr->translator_name == L( 'e', 'n' ) ) && chars::is_alpha( w.first_char ) && ( w.first_char != 'i' ) ) {
		// English Specific !!!!
		// any single letter before a dot is an abbreviation, except 'I'
		w.dictionary_flags[0] |= FLAG_ALLOW_DOT;
	}

	if ( ( tr->langopts.param[LOPT_ALT] & 2 ) && ( ( w.dictionary_flags[0] & ( FLAG_ALT_TRANS | FLAG_ALT2_TRANS ) ) != 0 ) ) {
		ApplySpecialAttribute2( tr, w.word_phonemes_arg, w.dictionary_flags[0] );
	}

	w.dictionary_flags[0] |= w.was_unpronouncable;
	memcpy( w.word_start, w.word_copy2, w.word_copy_length );
	return w.dictionary_flags[0];
}


void Engine::ApplySpecialAttribute2( Translator* tr, char* phonemes, int dict_flags ) {
	// apply after the translation is complete
	int len;


	len = strlen( phonemes );

	if ( tr->langopts.param[LOPT_ALT] & 2 ) {
		for ( int ix = 0; ix < ( len - 1 ); ix++ ) {
			if ( phonemes[ix] == phonSTRESS_P ) {
				char* p;
				p = &phonemes[ix + 1];
				if ( ( dict_flags & FLAG_ALT2_TRANS ) != 0 ) {
					if ( *p == PhonemeCode( 'E' ) ) {
						*p = PhonemeCode( 'e' );
					}
					if ( *p == PhonemeCode( 'O' ) ) {
						*p = PhonemeCode( 'o' );
					}
				} else {
					if ( *p == PhonemeCode( 'e' ) ) {
						*p = PhonemeCode( 'E' );
					}
					if ( *p == PhonemeCode( 'o' ) ) {
						*p = PhonemeCode( 'O' );
					}
				}
				break;
			}
		}
	}
}


void Engine::ChangeWordStress( Translator* tr, char* word, int new_stress ) {
	int ix;
	unsigned char* p;
	int max_stress;
	int vowel_count;		   // num of vowels + 1
	int stressed_syllable = 0; // position of stressed syllable
	unsigned char phonetic[N_WORD_PHONEMES];
	signed char vowel_stress[N_WORD_PHONEMES / 2];

	strcpy( (char*)phonetic, word );
	max_stress = GetVowelStress( tr, phonetic, vowel_stress, &vowel_count, &stressed_syllable, 0 );

	if ( new_stress >= STRESS_IS_PRIMARY ) {
		// promote to primary stress
		for ( ix = 1; ix < vowel_count; ix++ ) {
			if ( vowel_stress[ix] >= max_stress ) {
				vowel_stress[ix] = new_stress;
				break;
			}
		}
	} else {
		// remove primary stress
		for ( ix = 1; ix < vowel_count; ix++ ) {
			if ( vowel_stress[ix] > new_stress ) { // >= allows for diminished stress (=1)
				vowel_stress[ix] = new_stress;
			}
		}
	}

	// write out phonemes
	ix = 1;
	p = phonetic;
	while ( *p != 0 ) {
		if ( ( phoneme_tab[*p]->type == phVOWEL ) && !( phoneme_tab[*p]->phflags & phNONSYLLABIC ) ) {
			if ( ( vowel_stress[ix] == STRESS_IS_DIMINISHED ) || ( vowel_stress[ix] > STRESS_IS_UNSTRESSED ) ) {
				*word++ = stress_phonemes[(unsigned char)vowel_stress[ix]];
			}

			ix++;
		}
		*word++ = *p++;
	}
	*word = 0;
}

char* Engine::SpeakIndividualLetters( Translator* tr, char* word, char* phonemes, int spell_word, const ALPHABET* current_alphabet_arg,
									  char word_phonemes_arg[] ) {
	int posn = 0;
	int capitals = 0;
	bool non_initial = false;

	if ( spell_word > 2 ) {
		capitals = 2; // speak 'capital'
	}
	if ( spell_word > 1 ) {
		capitals |= 4; // speak character code for unknown letters
	}

	while ( ( *word != ' ' ) && ( *word != 0 ) ) {
		word += TranslateLetter( tr, word, phonemes, capitals | non_initial, current_alphabet_arg );
		posn++;
		non_initial = true;
		if ( phonemes[0] == phonSWITCH ) {
			// change to another language in order to translate this word
			strcpy( word_phonemes_arg, phonemes );
			return nullptr;
		}
	}
	SetSpellingStress( tr, phonemes, spell_word, posn );
	return word;
}


static const char* const hex_letters[] = { "'e:j", "b'i:", "s'i:", "d'i:", "'i:", "'ef" };
static const char* const modifiers[] = { nullptr, "_sub", "_sup", nullptr };
// unicode ranges for non-ascii digits 0-9 (these must be in ascending order)
static const int number_ranges[] = { 0x660, 0x6f0,													  // arabic
									 0x966, 0x9e6, 0xa66, 0xae6,  0xb66,  0xbe6, 0xc66, 0xce6, 0xd66, // indic
									 0xe50, 0xed0, 0xf20, 0x1040, 0x1090, 0 };


int Engine::TranslateLetter( Translator* tr, char* word, char* phonemes, int control, const ALPHABET* current_alphabet_arg ) {
	// get pronunciation for an isolated letter
	// return number of bytes used by the letter
	// control bit 0:  a non-initial letter in a word
	//         bit 1:  say 'capital'
	//         bit 2:  say character code for unknown letters

	int n_bytes;
	int letter;
	int len;
	const ALPHABET* alphabet;
	int al_offset;
	int al_flags;
	int number;
	int phontab_1;
	char capital[30];
	char ph_buf[80];
	char ph_buf2[80];
	char ph_alphabet[80];
	char hexbuf[12];
	static const char pause_string[] = { phonPAUSE, 0 };

	ph_buf[0] = 0;
	ph_alphabet[0] = 0;
	capital[0] = 0;
	phontab_1 = translator->phoneme_tab_ix;

	n_bytes = utf8_in( &letter, word );

	if ( ( letter & 0xfff00 ) == 0x0e000 ) {
		letter &= 0xff; // uncode private usage area
	}

	if ( control & 2 ) {
		// include CAPITAL information
		if ( chars::is_upper( letter ) ) {
			Lookup( tr, "_cap", capital );
		}
	}
	letter = towlower2( letter, tr );
	LookupLetter( tr, letter, word[n_bytes], ph_buf, control & 1 );

	if ( ph_buf[0] == 0 ) {
		// is this a subscript or superscript letter ?
		int c;
		if ( ( c = IsSuperscript( letter ) ) != 0 ) {
			letter = c & 0x3fff;

			const char* modifier;
			if ( ( control & 4 ) && ( ( modifier = modifiers[c >> 14] ) != nullptr ) ) {
				// don't say "superscript" during normal text reading
				Lookup( tr, modifier, capital );
				if ( capital[0] == 0 ) {
					capital[2] = SetTranslator3( ESPEAKNG_DEFAULT_VOICE ); // overwrites previous contents of translator3
					Lookup( translator3, modifier, &capital[3] );
					if ( capital[3] != 0 ) {
						capital[0] = phonPAUSE;
						capital[1] = phonSWITCH;
						len = strlen( &capital[3] );
						capital[len + 3] = phonSWITCH;
						capital[len + 4] = phontab_1;
						capital[len + 5] = 0;
					}
				}
			}
		}
		LookupLetter( tr, letter, word[n_bytes], ph_buf, control & 1 );
	}

	if ( ph_buf[0] == phonSWITCH ) {
		strcpy( phonemes, ph_buf );
		return 0;
	}


	if ( ( ph_buf[0] == 0 ) && ( ( number = NonAsciiNumber( letter ) ) > 0 ) ) {
		// convert a non-ascii number to 0-9
		LookupLetter( tr, number, 0, ph_buf, control & 1 );
	}

	al_offset = 0;
	al_flags = 0;
	if ( ( alphabet = AlphabetFromChar( letter ) ) != nullptr ) {
		al_offset = alphabet->offset;
		al_flags = alphabet->flags;
	}

	if ( alphabet != current_alphabet_arg ) {
		// speak the name of the alphabet
		current_alphabet_arg = alphabet;
		if ( ( alphabet != nullptr ) && !( al_flags & AL_DONT_NAME ) && ( al_offset != translator->letter_bits_offset ) ) {
			if ( ( al_flags & AL_DONT_NAME ) || ( al_offset == translator->langopts.alt_alphabet ) ||
				 ( al_offset == translator->langopts.our_alphabet ) ) {
				// don't say the alphabet name
			} else {
				ph_buf2[0] = 0;
				if ( Lookup( translator, alphabet->name, ph_alphabet ) == 0 ) { // the original language for the current voice
					// Can't find the local name for this alphabet, use the English name
					ph_alphabet[2] = SetTranslator3( ESPEAKNG_DEFAULT_VOICE ); // overwrites previous contents of translator3
					Lookup( translator3, alphabet->name, ph_buf2 );
				} else if ( translator != tr ) {
					phontab_1 = tr->phoneme_tab_ix;
					strcpy( ph_buf2, ph_alphabet );
					ph_alphabet[2] = translator->phoneme_tab_ix;
				}

				if ( ph_buf2[0] != 0 ) {
					// we used a different language for the alphabet name (now in ph_buf2)
					ph_alphabet[0] = phonPAUSE;
					ph_alphabet[1] = phonSWITCH;
					strcpy( &ph_alphabet[3], ph_buf2 );
					len = strlen( ph_buf2 ) + 3;
					ph_alphabet[len] = phonSWITCH;
					ph_alphabet[len + 1] = phontab_1;
					ph_alphabet[len + 2] = 0;
				}
			}
		}
	}

	// caution: SetWordStress() etc don't expect phonSWITCH + phoneme table number

	if ( ph_buf[0] == 0 ) {
		int language;
		if ( ( al_offset != 0 ) && ( al_offset == translator->langopts.alt_alphabet ) ) {
			language = translator->langopts.alt_alphabet_lang;
		} else if ( ( alphabet != nullptr ) && ( alphabet->language != 0 ) && !( al_flags & AL_NOT_LETTERS ) ) {
			language = alphabet->language;
		} else {
			language = L( 'e', 'n' );
		}

		if ( ( language != tr->translator_name ) || ( language == L( 'k', 'o' ) ) ) {
			char* p3;
			//int initial, code;
			char hangul_buf[12];

			// speak in the language for this alphabet (or English)
			char word_buf[5];
			ph_buf[2] = SetTranslator3( WordToString2( word_buf, language ) );

			if ( translator3 != nullptr ) {
				int code;
				if ( ( ( code = letter - 0xac00 ) >= 0 ) && ( letter <= 0xd7af ) ) {
					// Special case for Korean letters.
					// break a syllable hangul into 2 or 3 individual jamo

					hangul_buf[0] = ' ';
					p3 = &hangul_buf[1];
					int initial;
					if ( ( initial = ( code / 28 ) / 21 ) != 11 ) {
						p3 += utf8_out( initial + 0x1100, p3 );
					}
					utf8_out( ( ( code / 28 ) % 21 ) + 0x1161, p3 ); // medial
					utf8_out( ( code % 28 ) + 0x11a7, &p3[3] );		 // final
					p3[6] = ' ';
					p3[7] = 0;
					ph_buf[3] = 0;
					TranslateRules( translator3, &hangul_buf[1], &ph_buf[3], sizeof( ph_buf ) - 3, nullptr, 0, nullptr );
					SetWordStress( translator3, &ph_buf[3], nullptr, -1, 0 );
				} else {
					LookupLetter( translator3, letter, word[n_bytes], &ph_buf[3], control & 1 );
				}

				if ( ph_buf[3] == phonSWITCH ) {
					// another level of language change
					ph_buf[2] = SetTranslator3( &ph_buf[4] );
					LookupLetter( translator3, letter, word[n_bytes], &ph_buf[3], control & 1 );
				}

				SelectPhonemeTable( voice_phoneme_tab_ix ); // revert to original phoneme table

				if ( ph_buf[3] != 0 ) {
					ph_buf[0] = phonPAUSE;
					ph_buf[1] = phonSWITCH;
					len = strlen( &ph_buf[3] ) + 3;
					ph_buf[len] = phonSWITCH; // switch back
					ph_buf[len + 1] = tr->phoneme_tab_ix;
					ph_buf[len + 2] = 0;
				}
			}
		}
	}

	if ( ph_buf[0] == 0 ) {
		// character name not found
		int speak_letter_number = 1;
		if ( !( al_flags & AL_NO_SYMBOL ) ) {
			if ( chars::is_alpha( letter ) ) {
				Lookup( translator, "_?A", ph_buf );
			}

			if ( ( ph_buf[0] == 0 ) && !chars::is_space( letter ) ) {
				Lookup( translator, "_??", ph_buf );
			}

			if ( ph_buf[0] == 0 ) {
				EncodePhonemes( "l'et@", ph_buf, nullptr );
			}
		}

		if ( !( control & 4 ) && ( al_flags & AL_NOT_CODE ) ) {
			// don't speak the character code number, unless we want full details of this character
			speak_letter_number = 0;
		}

		if ( speak_letter_number ) {
			char* p2;
			if ( al_offset == 0x2800 ) {
				// braille dots symbol, list the numbered dots
				p2 = hexbuf;
				for ( int ix = 0; ix < 8; ix++ ) {
					if ( letter & ( 1 << ix ) ) {
						*p2++ = '1' + ix;
					}
				}
				*p2 = 0;
			} else {
				// speak the hexadecimal number of the character code
				format( hexbuf, "%x", letter );
			}

			char* pbuf;
			pbuf = ph_buf;
			for ( p2 = hexbuf; *p2 != 0; p2++ ) {
				pbuf += strlen( pbuf );
				*pbuf++ = phonPAUSE_VSHORT;
				LookupLetter( translator, *p2, 0, pbuf, 1 );
				if ( ( ( pbuf[0] == 0 ) || ( pbuf[0] == phonSWITCH ) ) && ( *p2 >= 'a' ) ) {
					// This language has no translation for 'a' to 'f', speak English names using base phonemes
					EncodePhonemes( hex_letters[*p2 - 'a'], pbuf, nullptr );
				}
			}
			strcat( pbuf, pause_string );
		}
	}

	len = strlen( phonemes );

	if ( tr->langopts.accents & 2 ) { // 'capital' before or after the word ?
		format( ph_buf2, "%c%s%s%s", 0xff, ph_alphabet, ph_buf, capital );
	} else {
		format( ph_buf2, "%c%s%s%s", 0xff, ph_alphabet, capital, ph_buf ); // the 0xff marker will be removed or replaced in SetSpellingStress()
	}
	if ( ( len + strlen( ph_buf2 ) ) < N_WORD_PHONEMES ) {
		strcpy( &phonemes[len], ph_buf2 );
	}
	return n_bytes;
}

// append plural suffixes depending on preceding letter
void Engine::addPluralSuffixes( int flags, Translator* tr, char last_char, char* word_phonemes_arg ) {
	char word_zz[4] = { ' ', 'z', 'z', 0 };
	char word_iz[4] = { ' ', 'i', 'z', 0 };
	char word_ss[4] = { ' ', 's', 's', 0 };
	if ( flags & FLAG_HAS_PLURAL ) {
		// s or 's suffix, append [s], [z] or [Iz] depending on previous letter
		if ( last_char == 'f' ) {
			TranslateRules( tr, &word_ss[1], word_phonemes_arg, N_WORD_PHONEMES, nullptr, 0, nullptr );
		} else if ( ( last_char == 0 ) || ( strchr_w( "hsx", last_char ) == nullptr ) ) {
			TranslateRules( tr, &word_zz[1], word_phonemes_arg, N_WORD_PHONEMES, nullptr, 0, nullptr );
		} else {
			TranslateRules( tr, &word_iz[1], word_phonemes_arg, N_WORD_PHONEMES, nullptr, 0, nullptr );
		}
	}
}

int Engine::CheckDottedAbbrev( char* word1 ) {
	int wc;
	int count = 0;
	int ix;
	char* word;
	char* wbuf;
	char word_buf[80];

	word = word1;
	wbuf = word_buf;

	for ( ;; ) {
		int ok = 0;
		int nbytes = utf8_in( &wc, word );
		if ( ( word[nbytes] == ' ' ) && IsAlpha( wc ) ) {
			if ( word[nbytes + 1] == '.' ) {
				if ( word[nbytes + 2] == ' ' ) {
					ok = 1;
				} else if ( word[nbytes + 2] == '\'' && word[nbytes + 3] == 's' ) {
					nbytes += 2; // delete the final dot (eg. u.s.a.'s)
					ok = 2;
				}
			} else if ( ( count > 0 ) ) {
				ok = 2;
			}
		}

		if ( ok == 0 ) {
			break;
		}

		for ( ix = 0; ix < nbytes; ix++ ) {
			*wbuf++ = word[ix];
		}

		count++;

		if ( ok == 2 ) {
			word += nbytes;
			break;
		}

		word += ( nbytes + 3 );
	}

	if ( count > 1 ) {
		ix = wbuf - word_buf;
		memcpy( word1, word_buf, ix );
		while ( &word1[ix] < word ) {
			word1[ix++] = ' ';
		}
		dictionary_skipwords = ( count - 1 ) * 2;
	}
	return count;
}

int Engine::NonAsciiNumber( int letter ) {
	// Change non-ascii digit into ascii digit '0' to '9', (or -1 if not)
	const int* p;
	int base;

	for ( p = number_ranges; ( base = *p ) != 0; p++ ) {
		if ( letter < base ) {
			break; // not found
		}
		if ( letter < ( base + 10 ) ) {
			return letter - base + '0';
		}
	}
	return -1;
}

int Engine::Unpronouncable( Translator* tr, char* word, int posn ) {
	/* Determines whether a word in 'unpronouncable', i.e. whether it should
	    be spoken as individual letters.

	    This function may be language specific. This is a generic version.
	 */

	int c;
	int c1 = 0;
	int vowel_posn = 9;
	int index;
	int count;
	const ALPHABET* alphabet;

	utf8_in( &c, word );
	if ( ( tr->letter_bits_offset > 0 ) && ( c < 0x241 ) ) {
		// Latin characters for a language with a non-latin alphabet
		return 0; // so we can re-translate the word as English
	}

	if ( ( ( alphabet = AlphabetFromChar( c ) ) != nullptr ) && ( alphabet->offset != tr->letter_bits_offset ) ) {
		// Character is not in our alphabet
		return 0;
	}

	if ( tr->langopts.param[LOPT_UNPRONOUNCABLE] == 1 ) {
		return 0;
	}

	if ( ( ( c = *word ) == ' ' ) || ( c == 0 ) || ( c == '\'' ) ) {
		return 0;
	}

	index = 0;
	count = 0;
	for ( ;; ) {
		index += utf8_in( &c, &word[index] );
		if ( ( c == 0 ) || ( c == ' ' ) ) {
			break;
		}

		if ( ( c == '\'' ) && ( ( count > 1 ) || ( posn > 0 ) ) ) {
			break; // "tv'" but not "l'"
		}

		if ( count == 0 ) {
			c1 = c;
		}

		if ( ( c == '\'' ) && ( tr->langopts.param[LOPT_UNPRONOUNCABLE] == 3 ) ) {
			// don't count apostrophe
		} else {
			count++;
		}

		if ( IsVowel( tr, c ) ) {
			vowel_posn = count; // position of the first vowel
			break;
		}

		if ( ( c != '\'' ) && !chars::is_alpha( c ) ) {
			return 0;
		}
	}

	if ( ( vowel_posn > 2 ) && ( tr->langopts.param[LOPT_UNPRONOUNCABLE] == 2 ) ) {
		// Lookup unpronounable rules in *_rules
		return Unpronouncable2( tr, word );
	}

	if ( c1 == tr->langopts.param[LOPT_UNPRONOUNCABLE] ) {
		vowel_posn--; // disregard this as the initial letter when counting
	}

	if ( vowel_posn > ( tr->langopts.max_initial_consonants + 1 ) ) {
		return 1; // no vowel, or no vowel in first few letters
	}

	return 0;
}

int Engine::Unpronouncable2( Translator* tr, char* word ) {
	int c;
	int end_flags;
	char ph_buf[N_WORD_PHONEMES];

	ph_buf[0] = 0;
	c = word[-1];
	word[-1] = ' '; // ensure there is a space before the "word"
	end_flags = TranslateRules( tr, word, ph_buf, sizeof( ph_buf ), nullptr, FLAG_UNPRON_TEST, nullptr );
	word[-1] = c;
	if ( ( end_flags == 0 ) || ( end_flags & SUFX_UNPRON ) ) {
		return 1;
	}
	return 0;
}

} // namespace propan2ol
