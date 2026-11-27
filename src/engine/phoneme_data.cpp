// Phoneme tables and phoneme programs, from espeak-ng's synthdata.c (LoadPhData, SetUpPhonemeTable, the phoneme lookups,
// InterpretCondition, InterpretPhoneme and InterpretPhoneme2).
//
// The tables (phontab), the program words (phonindex) and the program data (phondata) are read in place from the embedded
// blobs; nothing is copied. A phoneme's program is bytecode that rewrites the phoneme by stress and context (ChangeIf, conditions,
// jumps, calls) and carries its IPA name. Only what changes the phoneme string is interpreted: the formant, waveform, envelope
// and vowel-transition operands that drive synthesis are skipped over, exactly as far as upstream steps, so the
// instruction stream stays in step. Dropped with them: LookupSpect, GetEnvelope, FreePhData, the sound address / parameter
// outputs of InterpretPhoneme, and the KlattSynth / MbrolaSynth conditions (always false here).


#include "engine.h"
#include "../embedded_data.h"

namespace propan2ol {

constexpr int PHDATA_VERSION = 0x014801;


// ---- instruction layout ----
// An instruction word: bits 12-15 the group, then group-specific fields.
enum Group {
	GROUP_NO_OPERAND = 0,
	GROUP_CHANGE_IF = 1,
	GROUP_CONDITION = 2,
	GROUP_CONDITION_OR = 3,
	GROUP_JUMP = 6,
	GROUP_CALL = 9,
	GROUP_VOWEL_TRANSITION = 10,
	GROUP_FMT = 11,
	GROUP_WAV = 12,
	GROUP_VWLSTART = 13,
	GROUP_VWLEND = 14,
	GROUP_ADDWAV = 15,
};

constexpr int N_RETURN = 10; // depth of the call stack

PHONEME_TAB* Engine::table_of( int phcode ) { return phoneme_tab[phcode]; }

void Engine::set_up_table( int number ) {
	if ( const int includes = phoneme_tab_list[number].includes; includes > 0 ) {
		set_up_table( includes - 1 ); // the base tables first
	}
	PHONEME_TAB* phtab = phoneme_tab_list[number].phoneme_tab_ptr;
	for ( int ix = 0; ix < phoneme_tab_list[number].n_phonemes; ++ix ) {
		const int code = phtab[ix].code;
		phoneme_tab[code] = &phtab[ix];
		if ( code > n_phoneme_tab ) {
			std::memset( &phoneme_tab[n_phoneme_tab + 1], 0, ( code - ( n_phoneme_tab + 1 ) ) * sizeof( *phoneme_tab ) );
			n_phoneme_tab = code;
		}
	}
}

// The stress level that a ChangeIf / stress condition tests: a vowel's own, a consonant's from the following vowel.
bool Engine::stress_condition( Translator* tr, PHONEME_LIST* plist, int condition, int control ) {
	static constexpr int CONDITION_LEVEL[4] = { 1, 2, 4, 15 };

	PHONEME_LIST* pl;
	if ( table_of( plist[0].phcode )->type == phVOWEL ) {
		pl = plist;
	} else if ( table_of( plist[1].phcode )->type == phVOWEL ) {
		pl = &plist[1]; // a consonant takes the following vowel's stress
	} else {
		return false;
	}

	int stress_level = pl->stresslevel & 0xf;

	if ( tr != nullptr ) {
		if ( ( control & 1 ) && ( plist->synthflags & SFLAG_DICTIONARY ) && ( ( tr->langopts.param[LOPT_REDUCE] & 1 ) == 0 ) ) {
			return false; // don't change phonemes the dictionary gave for the word
		}
		if ( ( tr->langopts.param[LOPT_REDUCE] & 0x2 ) && ( stress_level >= pl->wordstress ) ) {
			stress_level = STRESS_IS_PRIMARY; // the most stressed syllable of an unstressed word counts as stressed
		}
	}

	if ( condition == STRESS_IS_PRIMARY ) {
		return stress_level >= pl->wordstress;
	}
	if ( condition == STRESS_IS_SECONDARY ) {
		return stress_level > STRESS_IS_SECONDARY;
	}
	return stress_level < CONDITION_LEVEL[condition];
}

int Engine::count_vowel_position( PHONEME_LIST* plist ) {
	int count = 0;
	for ( ;; ) {
		if ( plist->ph->type == phVOWEL ) {
			++count;
		}
		if ( plist->sourceix != 0 ) {
			break;
		}
		--plist;
	}
	return count;
}

// A condition instruction (2xxx / 3xxx). Bits 8-10 say which phoneme (0-5; 6: the 'which' is in the next word; 7: another
// condition); bit 11 clear: bits 0-7 are a phoneme code, set: bits 5-7 the kind of test and bits 0-4 its value.
bool Engine::interpret_condition( Translator* tr, int control, PHONEME_LIST* plist, const unsigned short* p_prog, WORD_PH_DATA* worddata ) {
	const int instn = *p_prog & 0xfff;
	unsigned data = instn & 0xff;
	const int instn2 = instn >> 8;

	if ( instn2 < 14 ) {
		PHONEME_LIST* plist_this = plist;
		int which = instn2 % 7;
		if ( which == 6 ) {
			which = *++p_prog;
		}

		// the word-boundary variants fail at a word start
		if ( which == 4 && plist[1].sourceix ) {
			return false;
		} // nextPhW
		if ( which == 5 && plist[0].sourceix ) {
			return false;
		} // prevPhW
		if ( which == 6 && ( plist[1].sourceix || plist[2].sourceix ) ) {
			return false;
		} // next2PhW

		bool check_endtype = false;
		switch ( which ) {
		case 0:
		case 5:
			--plist;
			check_endtype = true;
			break;	   // prevPh, prevPhW
		case 1: break; // thisPh
		case 2:
		case 4: ++plist; break; // nextPh, nextPhW
		case 3:
		case 6: plist += 2; break; // next2Ph, next2PhW
		case 7:					   // nextVowel, not across a word boundary
			for ( which = 1;; ++which ) {
				if ( plist[which].sourceix ) {
					return false;
				}
				if ( table_of( plist[which].phcode )->type == phVOWEL ) {
					plist = &plist[which];
					break;
				}
			}
			break;
		case 8: // prevVowel in this word
			if ( worddata == nullptr || worddata->prev_vowel.ph == nullptr ) {
				return false;
			}
			plist = &worddata->prev_vowel;
			check_endtype = true;
			break;
		case 9: // next3PhW
			for ( int ix = 1; ix <= 3; ++ix ) {
				if ( plist[ix].sourceix ) {
					return false;
				}
			}
			plist = &plist[3];
			break;
		case 10: // prev2PhW
			if ( plist[0].sourceix || plist[-1].sourceix ) {
				return false;
			}
			plist -= 2;
			check_endtype = true;
			break;
		}

		if ( ( which == 0 || which == 5 ) && plist->phcode == 1 ) {
			--plist; // a deleted phoneme (the nullptr phoneme): look at the one before
		}

		if ( control & 0x100 ) { // the "change phonemes" pass
			plist->ph = table_of( plist->phcode );
		}
		PHONEME_TAB* ph = plist->ph;

		if ( instn2 < 7 ) { // 'data' is a phoneme number
			if ( table_of( data )->mnemonic == ph->mnemonic ) {
				return true;
			}
			// not an exact match: a vowel type (e.g. #i)
			if ( check_endtype && ph->type == phVOWEL ) {
				return data == ph->end_type;
			} // prevPh() matches on end_type
			return data == ph->start_type; // thisPh() / nextPh() on start_type
		}

		data = instn & 0x1f;
		switch ( instn & 0xe0 ) {
		case CONDITION_IS_PHONEME_TYPE: return ph->type == data;
		case CONDITION_IS_PLACE_OF_ARTICULATION: return ( ( ph->phflags >> 16 ) & 0xf ) == data;
		case CONDITION_IS_PHFLAG_SET: return ( ph->phflags & ( 1 << data ) ) != 0;
		case CONDITION_IS_OTHER:
			switch ( data ) {
			case STRESS_IS_DIMINISHED:
			case STRESS_IS_UNSTRESSED:
			case STRESS_IS_NOT_STRESSED:
			case STRESS_IS_SECONDARY:
			case STRESS_IS_PRIMARY: return stress_condition( tr, plist, data, 0 );
			case isBreak: return ph->type == phPAUSE || ( plist_this->synthflags & SFLAG_NEXT_PAUSE );
			case isWordStart: return plist->sourceix != 0;
			case isWordEnd: return plist[1].sourceix || plist[1].ph->type == phPAUSE;
			case isAfterStress:
				if ( plist->sourceix != 0 ) {
					return false;
				}
				do {
					--plist;
					if ( ( plist->stresslevel & 0xf ) >= 4 ) {
						return true;
					}
				} while ( plist->sourceix == 0 );
				break;
			case isNotVowel: return ph->type != phVOWEL;
			case isFinalVowel:
				for ( ;; ) {
					++plist;
					if ( plist->sourceix != 0 ) {
						return true;
					} // the next word starts without another vowel
					if ( plist->ph->type == phVOWEL ) {
						return false;
					}
				}
			case isVoiced: return ph->type == phVOWEL || ph->type == phLIQUID || ( ph->phflags & phVOICED );
			case isFirstVowel: return count_vowel_position( plist ) == 1;
			case isSecondVowel: return count_vowel_position( plist ) == 2;
			case isTranslationGiven: return ( plist->synthflags & SFLAG_DICTIONARY ) != 0;
			}
			break;
		}
		return false;
	}

	if ( instn2 == 0xf && data == 1 ) {
		return control & 1;
	} // PreVoicing; (2) KlattSynth and (3) MbrolaSynth are never set here
	return false;
}

// The words an instruction takes.
int Engine::n_instn_words( const unsigned short* prog ) {
	static constexpr char FIXED_WORDS[16] = { 0, 1, 0, 0, 1, 1, 0, 1, 1, 2, 4, 0, 0, 0, 0, 0 };
	const int instn = *prog;
	const int group = instn >> 12;
	if ( FIXED_WORDS[group] > 0 ) {
		return FIXED_WORDS[group];
	}

	switch ( group ) {
	case 0:
		if ( ( ( instn & 0xf00 ) >> 8 ) == i_IPA_NAME ) {
			return ( ( instn & 0xff ) + 1 ) / 2 + 1;
		}
		return 1;
	case 6: {
		const int type2 = ( instn & 0xf00 ) >> 9;
		return type2 == 5 || type2 == 6 ? 12 : 1; // a switch on vowel type
	}
	case 2:
	case 3: {
		const int n = instn & 0x0f00;
		return n == 0x600 || n == 0x0d00 ? 2 : 1; // a condition, maybe the 2-word form
	}
	default: { // groups 11 to 15
		const int instn2 = prog[2];
		if ( ( instn2 >> 12 ) == 0xf ) {
			return 4;
		} // followed by addWav(): 2 more words
		return instn2 == INSTN_CONTINUE ? 3 : 2;
	}
	}
}


bool Engine::LoadPhData( void ) {
	propan2ol::Blob tab, index, data;
	if ( !propan2ol::find_blob( "phontab", tab ) || !propan2ol::find_blob( "phonindex", index ) || !propan2ol::find_blob( "phondata", data ) ) {
		return false;
	}
	phoneme_index = reinterpret_cast<const unsigned short*>( index.data );

	// phondata's first word is its format version
	int version = 0;
	for ( int ix = 0; ix < 4; ++ix ) {
		version += data.data[ix] << ( ix * 8 );
	}
	if ( version != PHDATA_VERSION ) {
		return false;
	}

	// the phoneme tables
	unsigned char* p = const_cast<unsigned char*>( tab.data );
	n_phoneme_tables = p[0];
	p += 4;
	for ( int ix = 0; ix < n_phoneme_tables; ++ix ) {
		const int n_phonemes = p[0];
		phoneme_tab_list[ix].n_phonemes = p[0];
		phoneme_tab_list[ix].includes = p[1];
		p += 4;
		std::memcpy( phoneme_tab_list[ix].name, p, N_PHONEME_TAB_NAME );
		p += N_PHONEME_TAB_NAME;
		phoneme_tab_list[ix].phoneme_tab_ptr = reinterpret_cast<PHONEME_TAB*>( p );
		p += n_phonemes * sizeof( PHONEME_TAB );
	}
	if ( phoneme_tab_number >= n_phoneme_tables ) {
		phoneme_tab_number = 0;
	}
	return true;
}

int Engine::PhonemeCode( unsigned int mnem ) {
	for ( int ix = 0; ix < n_phoneme_tab; ++ix ) {
		if ( phoneme_tab[ix] != nullptr && phoneme_tab[ix]->mnemonic == mnem ) {
			return phoneme_tab[ix]->code;
		}
	}
	return 0;
}

int Engine::LookupPhonemeString( const char* string ) {
	// up to 4 characters packed into a word, the first in the low byte
	unsigned int mnem = 0;
	for ( int ix = 0; ix < 4 && string[ix] != 0; ++ix ) {
		mnem |= static_cast<unsigned char>( string[ix] ) << ( ix * 8 );
	}
	return PhonemeCode( mnem );
}

void Engine::SelectPhonemeTable( int number ) {
	if ( current_phoneme_table == number ) {
		return;
	}
	n_phoneme_tab = 0;
	set_up_table( number ); // recursively for included tables
	++n_phoneme_tab;
	current_phoneme_table = number;
}

int Engine::LookupPhonemeTable( const char* name ) {
	for ( int ix = 0; ix < n_phoneme_tables; ++ix ) {
		if ( std::strcmp( name, phoneme_tab_list[ix].name ) == 0 ) {
			phoneme_tab_number = ix;
			return ix;
		}
	}
	return -1;
}

int Engine::SelectPhonemeTableName( const char* name ) {
	const int ix = LookupPhonemeTable( name );
	if ( ix != -1 ) {
		SelectPhonemeTable( ix );
	}
	return ix;
}

// control bit 0: PreVoicing; bit 8: the "change phonemes" pass.
void Engine::InterpretPhoneme( Translator* tr, int control, PHONEME_LIST* plist, PHONEME_DATA* phdata, WORD_PH_DATA* worddata ) {
	PHONEME_TAB* ph = plist->ph;

	if ( worddata != nullptr && plist->sourceix ) {
		worddata->prev_vowel.ph = nullptr;
	} // a word starts

	std::memset( phdata, 0, sizeof( PHONEME_DATA ) );
	phdata->pd_param[i_SET_LENGTH] = ph->std_length;
	phdata->pd_param[i_LENGTH_MOD] = ph->length_mod;
	if ( ph->program == 0 ) {
		return;
	}

	const unsigned short* return_addr[N_RETURN];
	int n_return = 0;
	int end_flag = 0;

	for ( const unsigned short* prog = &phoneme_index[ph->program]; end_flag != 1; ++prog ) {
		unsigned short instn = *prog;
		int instn2 = ( instn >> 8 ) & 0xf;

		switch ( instn >> 12 ) {
		case GROUP_NO_OPERAND: {
			int data = instn & 0xff;
			if ( instn2 == 0 ) {
				if ( data == INSTN_RETURN ) {
					end_flag = 1;
				} // (CONTINUE does nothing, and nothing else is valid)
			} else if ( instn2 == i_APPEND_IFNEXTVOWEL ) {
				if ( table_of( plist[1].phcode )->type == phVOWEL ) {
					phdata->pd_param[i_APPEND_PHONEME] = data;
				}
			} else if ( instn2 == i_ADD_LENGTH ) {
				if ( data & 0x80 ) {
					data = -( 0x100 - data );
				} // a negative value
				phdata->pd_param[i_SET_LENGTH] += data;
			} else if ( instn2 == i_IPA_NAME ) {
				// UTF-8 characters, 2 per instruction word
				int ix = 0;
				for ( ; ix < data && ix < 16; ix += 2 ) {
					++prog;
					phdata->ipa_string[ix] = prog[0] >> 8;
					phdata->ipa_string[ix + 1] = prog[0] & 0xff;
				}
				phdata->ipa_string[ix] = 0;
			} else if ( instn2 < N_PHONEME_DATA_PARAM ) {
				phdata->pd_param[instn2] = data;
				if ( instn2 == i_CHANGE_PHONEME && ( control & 0x100 ) ) {
					end_flag = 1;
				} // ChangePhoneme() in the change pass
			}
			break;
		}
		case GROUP_CHANGE_IF:
			if ( tr != nullptr && instn2 < 8 && stress_condition( tr, plist, instn2 & 7, 1 ) ) {
				phdata->pd_param[i_CHANGE_PHONEME] = instn & 0xff;
				end_flag = 1;
			}
			break;
		case GROUP_CONDITION:
		case GROUP_CONDITION_OR: {
			// a sequence of conditions, accumulated with and / or
			bool or_flag = false, truth = true;
			while ( ( instn & 0xe000 ) == 0x2000 ) {
				bool truth2 = interpret_condition( tr, control, plist, prog, worddata );
				prog += n_instn_words( prog );
				if ( *prog == i_NOT ) {
					truth2 = !truth2;
					++prog;
				}
				truth = or_flag ? ( truth || truth2 ) : ( truth && truth2 );
				or_flag = ( instn & 0x1000 ) != 0;
				instn = *prog;
			}
			if ( !truth ) {
				if ( ( instn & 0xf800 ) == i_JUMP_FALSE ) {
					prog += instn & 0xff;
				} else {
					// not followed by JUMP_FALSE: skip the instruction, and an ELSE jump after it
					prog += n_instn_words( prog );
					if ( ( prog[0] & 0xfe00 ) == 0x6000 ) {
						++prog;
					}
				}
			}
			--prog;
			break;
		}
		case GROUP_JUMP:
			switch ( instn2 >> 1 ) {
			case 0: prog += ( instn & 0xff ) - 1; break;
			case 5:
			case 6:
				prog += 12;
				break; // switch on the next / previous vowel's type: a synthesis table, skipped (upstream advances 12, then the loop 1)
			} // (case 4, the conditional jumps, were handled with the conditions)
			break;
		case GROUP_CALL: {
			const int data = ( ( instn & 0xf ) << 16 ) + prog[1];
			++prog;
			if ( instn2 == 1 && n_return < N_RETURN ) { // call a procedure or another phoneme (2, 3: pitch / amplitude envelope, skipped)
				return_addr[n_return++] = prog;
				prog = &phoneme_index[data] - 1;
			}
			break;
		}
		case GROUP_VOWEL_TRANSITION: prog += 3; break;
		case GROUP_FMT:
		case GROUP_WAV:
		case GROUP_VWLSTART:
		case GROUP_VWLEND:
		case GROUP_ADDWAV: {
			// the synthesis operands (addresses, parameters) are skipped; the return logic still follows the stream
			const int kind = ( instn >> 12 ) - 11;
			++prog;
			if ( prog[1] != INSTN_CONTINUE ) {
				if ( kind < 2 ) { // FMT() and WAV() imply Return
					end_flag = 1;
					if ( ( prog[1] >> 12 ) == 0xf ) {
						end_flag = 2;
					} // return after the following addWav()
				} else if ( kind == pd_ADDWAV ) {
					--end_flag; // addWav(): return if the previous instruction was FMT() or WAV()
				}
			}
			break;
		}
		default: break; // an invalid instruction (upstream reports it on stderr)
		}

		if ( end_flag == 1 && n_return > 0 ) { // return from a called procedure or phoneme
			end_flag = 0;
			prog = return_addr[--n_return];
		}
	}

	if ( worddata != nullptr && plist->type == phVOWEL ) {
		std::memcpy( &worddata->prev_vowel, &plist[0], sizeof( PHONEME_LIST ) );
	}
}

// The program of a single isolated phoneme: the context is pauses around it.
void Engine::InterpretPhoneme2( int phcode, PHONEME_DATA* phdata ) {
	PHONEME_LIST plist[4];
	std::memset( plist, 0, sizeof( plist ) );
	for ( PHONEME_LIST& pl : plist ) {
		pl.phcode = phonPAUSE;
		pl.ph = phoneme_tab[phonPAUSE];
	}
	plist[1].phcode = phcode;
	plist[1].ph = phoneme_tab[phcode];
	plist[2].sourceix = 1;
	InterpretPhoneme( nullptr, 0, &plist[1], phdata, nullptr );
}

} // namespace propan2ol
