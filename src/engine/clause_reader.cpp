// The clause reader, from espeak-ng's readclause.c: reads text up to the end of one clause (a sentence or part of one), applies the
// language's character ignore / replace table, recognises clause-ending punctuation, and writes the clause to a buffer as UTF-8 with
// a map from each byte back to its source character. Returns the clause type: pause (x10 ms) in bits 0-11, intonation in bits 12-14.
//
// Not carried over, because nothing on the text-to-phonemes path sets what they serve: SSML (tags, entities, the voice / parameter
// stacks), say-as modes, phoneme-mnemonic input ([[...]]), the speak-punctuation option and its sound icons, capital-letter
// announcement, line-length and single-line modes, skip / end character positions, and the marker name table. One behaviour
// differs from upstream: the embedded-command prefix U+0001 (with a V or B after it, a voice change or the punctuation option) is
// not interpreted; those characters pass through as text.


// One character at a time from the decoder, with one character of push-back.

#include "engine.h"

namespace propan2ol {

bool Engine::at_end() { return ungot_char == 0 && decoder.eof(); }

int Engine::get_char() {
	if ( const int c = ungot_char; c != 0 ) {
		ungot_char = 0;
		return c;
	}
	++count_characters;
	return static_cast<int>( decoder.get() );
}

void Engine::unget_char( int c ) { ungot_char = c; }

bool Engine::is_roman_upper( unsigned c ) { return c == 'I' || c == 'V' || c == 'X' || c == 'L'; }

// End the clause text: a space and the terminator, and give back the character that was read ahead.
void Engine::terminate( char* buf, int index, const int* unget ) {
	buf[index] = ' ';
	buf[index + 1] = 0;
	if ( unget != nullptr ) {
		unget_char( *unget );
	}
}

// The language's replacement for a character (0: none; 1: delete it).
int Engine::replacement( const unsigned short* list, int c ) {
	for ( int ix = 0; list[ix] != 0; ix += 2 ) {
		if ( list[ix] == c ) {
			return list[ix + 1];
		}
	}
	return 0;
}

// True if the character is to be ignored; else c1 may have been replaced.
bool Engine::ignore_or_replace( Translator* tr, int& c1 ) {
	if ( const int i = replacement( tr->chars_ignore, c1 ); i != 0 ) {
		if ( i == 1 ) {
			return true;
		}
		c1 = i;
	}
	return false;
}

// The spoken name of a character from the language's dictionary (the "only" mode of upstream's LookupCharName: an entry for the
// character itself), as phoneme text "[\002...]]" that translate.c reads back. An empty string if there is none.
void Engine::lookup_char_name( char buf[60], Translator* tr, int c ) {
	unsigned int flags[2] = { 0, 0 };
	char single_letter[24];
	char phonemes[60];
	single_letter[0] = 0;
	single_letter[1] = '_';
	const int n = text::utf8_out( c, &single_letter[2] );
	single_letter[2 + n] = 0;

	buf[0] = 0;
	char* string = &single_letter[2];
	LookupDictList( tr, &string, phonemes, flags, 0, nullptr );
	if ( phonemes[0] ) {
		char decoded[55];
		SetWordStress( tr, phonemes, flags, -1, 0 );
		DecodePhonemes( phonemes, decoded );
		std::snprintf( buf, 60, "[\002%s]]", decoded );
	}
}


int Engine::clause_type_from_codepoint( uint32_t c ) { return chars::clause_type( c ); }

int Engine::Eof( void ) { return at_end() ? 1 : 0; }

// A language mnemonic word (up to 4 characters, the first in the high byte) as a string.
const char* Engine::WordToString2( char buf[5], unsigned int word ) {
	char* p = buf;
	for ( int ix = 3; ix >= 0; --ix ) {
		if ( ( *p = static_cast<char>( word >> ( ix * 8 ) ) ) != 0 ) {
			++p;
		}
	}
	*p = 0;
	return buf;
}

// Does not end a clause at punctuation immediately followed by a letter or digit (1.23, !Speak, :path) or in repeated punctuation
// (..., !!!). A blank line is a paragraph break and also ends one.
int Engine::ReadClause( Translator* tr, char* buf, short* charix, int* charix_top, int n_buf, int* tone_type, char* voice_change ) {
	int c1 = ' ';	 // the current character
	int c2;			 // the next
	int cprev = ' '; // the previous
	int c_next = 0;
	int ix = 0;
	bool any_alnum = false;
	int announced_punctuation = 0; // the character whose name was written instead, so it isn't spoken twice
	bool stressed_word = false;

	tr->phonemes_repeat_count = 0;
	tr->clause_upper_count = 0;
	tr->clause_lower_count = 0;
	*tone_type = 0;
	*voice_change = 0;

	if ( ungot_char2 != 0 ) {
		c2 = ungot_char2;
	} else if ( at_end() ) {
		c2 = 0;
	} else {
		c2 = get_char();
	}

	while ( !at_end() || ungot_char != 0 || ungot_char2 != 0 ) {
		const int cprev2 = cprev;
		cprev = c1;
		c1 = c2;

		if ( at_end() ) {
			c2 = ' ';
		} else {
			c2 = get_char();
		}
		ungot_char2 = 0;

		if ( ignore_or_replace( tr, c1 ) ) {
			continue;
		}

		if ( chars::is_alnum( c1 ) ) {
			any_alnum = true;
		} else {
			if ( stressed_word ) {
				stressed_word = false;
				c1 = CHAR_EMPHASIS; // this word is stressed
				unget_char( c2 );
				c2 = ' ';
			}
			if ( c1 == 0xf0b ) {
				c1 = ' ';
			} // Tibetan inter-syllabic mark
			if ( c1 == 0xd4d && c2 == 0x200d ) {
				c1 = 0xd4e;
			} // Malayalam virama before a zero-width joiner: chillu-virama
		}

		if ( chars::is_upper( c1 ) ) {
			++tr->clause_upper_count;
		} else if ( chars::is_alpha( c1 ) ) {
			++tr->clause_lower_count;
		}

		if ( c1 == '\n' ) {
			// count consecutive newlines, ignoring other spaces
			int parag = 0;
			while ( !at_end() && chars::is_space( c2 ) ) {
				if ( c2 == '\n' ) {
					++parag;
				}
				c2 = get_char();
			}
			if ( parag > 0 ) {
				// a second newline: a paragraph; several blank lines, a longer pause
				terminate( buf, ix, &c2 );
				if ( parag > 3 ) {
					parag = 3;
				}
				return ( CLAUSE_PARAGRAPH - 30 ) + 30 * parag;
			}
		}

		announced_punctuation = 0;

		bool is_end_clause = false;

		if ( c1 == '.' && c2 == '.' ) {
			while ( !at_end() && ( c_next = get_char() ) == '.' ) { // 3 or more dots: an ellipsis
				c1 = 0x2026;
				c2 = ' ';
			}
			if ( c1 == 0x2026 ) {
				c2 = c_next;
			} else {
				unget_char( c_next );
			}
		}

		int punct_data = clause_type_from_codepoint( c1 );
		if ( punct_data != CLAUSE_NONE ) {
			// a run of ? and ! (??!?, !!??!): only the first decides
			if ( punct_data & ( CLAUSE_QUESTION | CLAUSE_EXCLAMATION ) ) {
				while ( !at_end() && ( clause_type_from_codepoint( c2 ) & ( CLAUSE_QUESTION | CLAUSE_EXCLAMATION ) ) ) {
					c_next = get_char();
					c2 = c_next;
				}
			}

			if ( punct_data & CLAUSE_PUNCTUATION_IN_WORD ) { // Armenian punctuation inside a word
				stressed_word = true;
				*tone_type = punct_data >> 12 & 0xf; // overrides the end-of-sentence type
				continue;
			}

			// ('-' is not checked: it would stop ":-)" being recognised; '?' is for a smart quote that was replaced by '?')
			if ( chars::is_space( c2 ) || ( punct_data & CLAUSE_OPTIONAL_SPACE_AFTER ) || text::is_bracket( c2 ) || c2 == '?' || at_end() ||
				 c2 == CTRL_EMBEDDED ) {
				is_end_clause = true;
			}
		}

		if ( punct_data & CLAUSE_SPEAK_PUNCTUATION_NAME ) {
			// the ellipsis, if the language's list gives it a pronunciation
			char* p2 = &buf[ix];
			char name[60];
			lookup_char_name( name, tr, c1 );
			std::strcpy( p2, name );
			if ( p2[0] != 0 ) {
				ix += static_cast<int>( std::strlen( p2 ) );
				announced_punctuation = c1;
				punct_data &= ~CLAUSE_INTONATION_TYPE; // intonation type 0: full stop
			}
		}

		if ( is_end_clause ) {
			int nl_count = 0;
			c_next = c2;
			if ( chars::is_space( c_next ) ) {
				while ( !at_end() && chars::is_space( c_next ) ) {
					if ( c_next == '\n' ) {
						++nl_count;
					}
					c_next = get_char(); // skip past the spaces
				}
			}

			if ( c1 == '.' && nl_count < 2 ) {
				punct_data |= CLAUSE_DOT_AFTER_LAST_WORD;
			}

			if ( nl_count == 0 ) {
				if ( c1 == ',' && cprev == '.' && tr->translator_name == L( 'h', 'u' ) && chars::is_digit( cprev2 ) &&
					 ( chars::is_digit( c_next ) || chars::is_lower( c_next ) ) ) {
					// Hungarian: "december 2., szerda": no break at the ',' after an ordinal number
					c1 = CHAR_COMMA_BREAK;
					is_end_clause = false;
				}

				if ( c1 == '.' && c_next == '\'' && decoder.peek() == 's' ) {
					is_end_clause = false; // an English acronym with a genitive, u.s.a.'s; but not if anything else follows the apostrophe
				}

				if ( c1 == '.' ) {
					if ( ( tr->langopts.numbers & NUM_ORDINAL_DOT ) &&
						 ( chars::is_digit( cprev ) || ( is_roman_upper( cprev ) && ( is_roman_upper( cprev2 ) || chars::is_space( cprev2 ) ) ) ) ) {
						// Hungarian: a dot after a number is an ordinal
						if ( !chars::is_digit( cprev ) ) {
							is_end_clause = false; // a Roman number followed by a dot
						} else if ( chars::is_lower( c_next ) || c_next == '-' ) {
							is_end_clause = false; // only before a lower-case letter (the hyphen is for 2.-kal)
						}
					}
					if ( chars::is_lower( c_next ) && !tr->langopts.lowercase_sentence ) {
						is_end_clause = false; // no capital letter next: probably an abbreviation
					}
					if ( !any_alnum ) {
						c1 = ' '; // no letters or digits yet: probably not a terminator
						is_end_clause = false;
					}
				} else if ( !any_alnum ) {
					is_end_clause = false;
				}
			}

			if ( is_end_clause ) {
				terminate( buf, ix, &c_next );

				if ( chars::is_digit( cprev ) && !text::is_alpha( c_next ) ) {
					punct_data &= ~CLAUSE_DOT_AFTER_LAST_WORD;
				}
				if ( nl_count > 1 ) {
					if ( punct_data == CLAUSE_QUESTION || punct_data == CLAUSE_EXCLAMATION ) {
						return punct_data + 35;
					} // a longer pause
					return CLAUSE_PARAGRAPH;
				}
				return punct_data; // punctuation counts only when followed by a blank or a bracket / quote
			} else if ( !at_end() && chars::is_space( c2 ) ) {
				unget_char( c_next );
			}
		}

		if ( c1 == announced_punctuation ) {
			// already announced: not spoken a second time, unless it's a hyphen (which TranslateClause() uses). This also turns the
			// decoder's terminating NUL into a space.
			if ( text::is_bracket( c1 ) ) {
				c1 = 0xe000 + '('; // private use: tells TranslateRules() the bracket's name has been spoken
			} else if ( c1 != '-' ) {
				c1 = ' ';
			}
		}

		int j = ix + 1;
		if ( c1 == 0xe000 + '<' ) {
			c1 = '<';
		}

		ix += text::utf8_out( c1, &buf[ix] );
		if ( !chars::is_space( c1 ) && !text::is_bracket( c1 ) ) {
			charix[ix] = count_characters - clause_start_char;
			while ( j < ix ) {
				charix[j++] = -1;
			} // the later bytes of a multibyte character
		}
		*charix_top = ix;

		if ( ( ix > n_buf - 75 && !text::is_alpha( c1 ) && !chars::is_digit( c1 ) ) || ix >= n_buf - 4 ) {
			// near the end of the buffer: break here, at a word boundary if possible ((n_buf-4) leaves room for a 3-byte character)
			terminate( buf, ix, &c2 );
			return CLAUSE_NONE;
		}
	}

	if ( stressed_word ) {
		ix += text::utf8_out( CHAR_EMPHASIS, &buf[ix] );
	}
	terminate( buf, ix, nullptr );
	return CLAUSE_EOF;
}

} // namespace propan2ol
