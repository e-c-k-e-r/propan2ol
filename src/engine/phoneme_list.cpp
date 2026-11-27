// MakePhonemeList and its helpers, from espeak-ng's phonemelist.c: the second stage of text to phonemes. The first stage (translate.c)
// leaves one clause's phonemes in ph_list2; this stage promotes a stress, drops redundant language switches, applies regressive
// voicing and the language's `replace` rules, then walks the list once running each phoneme's program (so stress and context
// can change it, insert a linking phoneme, or delete it) and writes the result to phoneme_list, which the IPA writer reads.
//
// Behaviour is upstream's, quirks included (noted where they are). What is gone: the synthesis-only fields of each list entry
// (envelope, length, pre-pause, amplitude, pitch), and the `wordgap` option, which only the synthesizer's parameter call sets.


#include "engine.h"

namespace propan2ol {

// the pause a word gap of 1-7 inserts
constexpr unsigned char GAP_PAUSE[8] = {
	0, phonPAUSE_VSHORT, phonPAUSE_SHORT, phonPAUSE, phonPAUSE_LONG, phonGLOTTALSTOP, phonPAUSE_LONG, phonPAUSE_LONG };

constexpr int MAX_ENTRIES = N_PHONEME_LIST - 3; // room kept for the terminating pauses

PHONEME_TAB* Engine::phoneme_of( int phcode ) { return phoneme_tab[phcode]; }

// Is the last word of the clause unstressed? Then give the nearest earlier word whose stress can be promoted its stress.
void Engine::promote_stress( PHONEME_LIST2* plist2, int n ) {
	int max_stress = 0;
	int j = n - 3; // start at the last phoneme before the terminating pauses and move back
	for ( ; j >= 0; --j ) {
		if ( ( plist2[j].stresslevel & 0x7f ) > max_stress ) {
			max_stress = plist2[j].stresslevel & 0x7f;
		}
		if ( plist2[j].sourceix != 0 ) {
			break;
		}
	}
	if ( max_stress >= 4 ) {
		return;
	}
	while ( --j >= 0 ) {
		if ( plist2[j].synthflags & SFLAG_PROMOTE_STRESS ) {
			plist2[j].stresslevel = 4;
			break;
		}
		if ( plist2[j].stresslevel >= 4 ) {
			break;
		} // a stressed syllable: stop looking
	}
}

// Mark phonemes that follow a switch to another phoneme table, and delete switches that change nothing: one to the table already
// current, or one straight followed by another switch. Returns the table current at the end of the clause.
int Engine::drop_redundant_switches( Translator* tr, PHONEME_LIST2* plist2 ) {
	int delete_count = 0;
	int current_table = tr->phoneme_tab_ix;
	int deleted_sourceix = -1;
	for ( int j = 0; j < n_ph_list2; ++j ) {
		if ( current_table != tr->phoneme_tab_ix ) {
			plist2[j].synthflags |= SFLAG_SWITCHED_LANG;
		}

		if ( delete_count > 0 ) {
			std::memcpy( &plist2[j - delete_count], &plist2[j], sizeof( plist2[0] ) );
			if ( deleted_sourceix != -1 ) {
				plist2[j - delete_count].sourceix = deleted_sourceix;
				deleted_sourceix = -1;
			}
		}

		if ( plist2[j].phcode == phonSWITCH ) {
			if ( !( plist2[j].synthflags & SFLAG_EMBEDDED ) && ( plist2[j].tone_ph == current_table || plist2[j + 1].phcode == phonSWITCH ||
																 ( plist2[j + 1].phcode == phonPAUSE && plist2[j + 2].phcode == phonSWITCH ) ) ) {
				if ( deleted_sourceix == -1 && plist2[j].sourceix != 0 ) {
					deleted_sourceix = plist2[j].sourceix;
				}
				++delete_count;
			} else {
				current_table = plist2[j].tone_ph;
			}
		}
	}
	n_ph_list2 -= delete_count;
	return current_table;
}

// Consonant clusters all voiced or all unvoiced, working backwards through the clause. `regression` is the language's
// LOPT_REGRESSIVE_VOICING: 0x1-0x8 the kind, 0x2 [v] and [R] stop the propagation, 0x4 stop at a word boundary,
// 0x8 propagate through liquids and nasals (Polish), 0x10 reset voicing at [v] / [R], 0x100 word-final devoicing.
void Engine::set_regressive_voicing( int regression, PHONEME_LIST2* plist2, Translator* tr ) {
	bool stop_propagation = false;
	int voicing = 0; // 1: unvoiced, 2: voiced

	for ( int j = n_ph_list2 - 1; j >= 0; --j ) {
		if ( plist2[j].phcode == phonSWITCH ) {
			// the table to go back to is the previous switch's
			int k = j - 1;
			while ( k >= 0 && plist2[k].phcode != phonSWITCH ) {
				--k;
			}
			SelectPhonemeTable( k >= 0 ? plist2[k].tone_ph : tr->phoneme_tab_ix );
		}
		PHONEME_TAB* ph = phoneme_of( plist2[j].phcode );
		if ( ph == nullptr ) {
			continue;
		}

		if ( plist2[j].synthflags & SFLAG_SWITCHED_LANG ) {
			stop_propagation = false;
			voicing = ( regression & 0x100 ) ? 1 : 0; // word-end devoicing
			continue;
		}

		const int type = ph->type;

		if ( regression & 0x2 ) {
			// [v], [v;] and [R^] don't cause regression
			if ( ( ph->mnemonic & 0xff ) == 'v' || ( ph->mnemonic & 0xff ) == 'R' ) {
				stop_propagation = true;
				if ( regression & 0x10 ) {
					voicing = 0;
				}
			}
		}

		if ( type == phSTOP || type == phFRICATIVE ) {
			if ( voicing == 0 && ( regression & 0xf ) ) {
				voicing = 1;
			} else if ( voicing == 2 && ph->end_type != 0 ) {
				plist2[j].phcode = ph->end_type; // consonants keep their voiced / unvoiced pair in end_type
			}
		} else if ( type == phVSTOP || type == phVFRICATIVE ) {
			if ( voicing == 0 && ( regression & 0xf ) ) {
				voicing = 2;
			} else if ( voicing == 1 && ph->end_type != 0 ) {
				plist2[j].phcode = ph->end_type;
			}
		} else if ( regression & 0x8 ) {
			if ( type == phPAUSE || type == phVOWEL ) {
				voicing = 0;
			} // Polish propagates through liquids and nasals
		} else {
			voicing = 0;
		}

		if ( stop_propagation ) {
			voicing = 0;
			stop_propagation = false;
		}

		if ( plist2[j].sourceix ) {
			if ( regression & 0x04 ) {
				voicing = 0;
			} // stop at a word boundary
			if ( ( regression & 0x100 ) && voicing == 0 ) {
				voicing = 1;
			} // devoice word-final consonants, unless propagating voiced
		}
	}
}

// Copy ph_list2 to `out` applying the voice's `replace` rules, except in text whose language was temporarily switched. A phoneme
// replaced by nullptr is dropped, its word-start marker moving to the next phoneme. Returns the count.
int Engine::substitute_phonemes( PHONEME_LIST* out ) {
	int n_out = 0;
	PHONEME_TAB* next = nullptr;
	int deleted_sourceix = -1;

	for ( int ix = 0; ix < n_ph_list2 && n_out < N_PHONEME_LIST; ++ix ) {
		PHONEME_LIST2* plist2 = &ph_list2[ix];
		if ( deleted_sourceix != -1 ) {
			plist2->sourceix = deleted_sourceix;
			deleted_sourceix = -1;
		}

		if ( plist2->phcode == phonSWITCH ) {
			SelectPhonemeTable( plist2->tone_ph );
		}

		if ( !( plist2->synthflags & SFLAG_SWITCHED_LANG ) ) {
			if ( ix < n_ph_list2 - 1 ) {
				next = phoneme_of( ph_list2[ix + 1].phcode );
			} // (stale at the last phoneme: upstream)

			const bool word_end = ix == n_ph_list2 - 1 || ( plist2 + 1 )->sourceix || ( next != nullptr && next->type == phPAUSE );

			for ( int k = 0; k < n_replace_phonemes; ++k ) {
				if ( plist2->phcode != replace_phonemes[k].old_ph ) {
					continue;
				}
				const int flags = replace_phonemes[k].type;
				if ( ( flags & 1 ) && !word_end ) {
					continue;
				} // only at the end of a word
				if ( ( flags & 2 ) && ( plist2->stresslevel & 0x7 ) > 3 ) {
					continue;
				} // not in stressed syllables
				if ( ( flags & 4 ) && plist2->sourceix == 0 ) {
					continue;
				} // only at the start of a word
				plist2->phcode = replace_phonemes[k].new_ph;
				if ( plist2->stresslevel > 1 && ( phoneme_of( plist2->phcode )->phflags & phUNSTRESSED ) ) {
					plist2->stresslevel = 0; // the replacement must be unstressed
				}
				break;
			}

			if ( plist2->phcode == 0 ) {
				deleted_sourceix = plist2->sourceix;
				continue; // replaced by nullptr: not copied
			}
		}

		std::memcpy( &out[n_out], plist2, sizeof( PHONEME_LIST2 ) );
		out[n_out].ph = phoneme_of( plist2->phcode );
		out[n_out].type = out[n_out].ph->type;
		++n_out;
	}
	return n_out;
}

// Each word's highest stress, written to every phoneme of the word. Returns the last word's, which upstream's later pass still
// reads as `word_stress` (the value is stale there: the pass below never updates it).
int Engine::mark_word_stress( PHONEME_LIST* list, int n ) {
	int word_stress = 0;
	for ( int j = 0; j < n; ) {
		if ( !list[j].sourceix ) {
			++j;
			continue;
		}
		word_stress = 0;
		int nextw = j;
		for ( ; nextw < n; ) {
			if ( list[nextw].stresslevel > word_stress ) {
				word_stress = list[nextw].stresslevel;
			}
			++nextw;
			if ( list[nextw].sourceix ) {
				break;
			} // the next word starts
		}
		for ( int k = j; k < nextw; ++k ) {
			list[k].wordstress = word_stress;
		}
		j = nextw;
	}
	return word_stress;
}

// After a phoneme has been changed: a vowel is a syllable (and starts unstressed if it wasn't one), then its program runs again.
// A second ChangePhoneme() is not obeyed.
void Engine::reinterpret( PHONEME_TAB* ph, PHONEME_TAB* old_ph, PHONEME_LIST* plist3, Translator* tr, PHONEME_DATA* phdata, WORD_PH_DATA* worddata ) {
	if ( ph->type == phVOWEL ) {
		plist3->synthflags |= SFLAG_SYLLABLE;
		if ( old_ph->type != phVOWEL ) {
			plist3->stresslevel = 0;
		} // a non-vowel became a vowel: unstressed
	} else {
		plist3->synthflags &= ~SFLAG_SYLLABLE;
	}
	InterpretPhoneme( tr, 0x100, plist3, phdata, worddata );
}

// Consecutive unstressed vowels, across word boundaries: alternate ones are reduced to 'diminished' stress. Run after the phoneme
// has taken its stress-dependent form.
void Engine::reduce_unstressed( Translator* tr, PHONEME_LIST* plist3, int word_stress, int& unstress_count ) {
	if ( plist3->stresslevel > 1 ) {
		unstress_count = 0;
		return;
	}
	++unstress_count;

	if ( tr->langopts.stress_flags & 0x08 ) {
		// consecutive unstressed vowels in unstressed words become diminished (marked TEST upstream)
		for ( PHONEME_LIST* p = plist3 + 1; p->type != phPAUSE; ++p ) {
			if ( p->type != phVOWEL ) {
				continue;
			}
			if ( p->stresslevel <= 1 ) {
				if ( plist3->wordstress < 4 ) {
					plist3->stresslevel = 0;
				}
				if ( p->wordstress < 4 ) {
					p->stresslevel = 0;
				}
			}
			break;
		}
	} else if ( unstress_count > 1 && ( unstress_count & 1 ) == 0 ) {
		// but not the last phoneme of a stressed word
		if ( ( tr->langopts.stress_flags & S_NO_DIM ) || ( word_stress > 3 && ( plist3 + 1 )->sourceix != 0 ) ) {
			unstress_count = 1; // an unstressed final vowel of a stressed word: try again at the next syllable
		} else {
			plist3->stresslevel = 0;
		}
	}
}

// At a word boundary: the pause the language puts there (vowel_pause and word_gap), as a phoneme to insert, or `insert_ph` unchanged.
int Engine::boundary_pause( Translator* tr, int insert_ph, PHONEME_TAB* ph, PHONEME_TAB* next, PHONEME_LIST* plist3, bool is_inserted, int ix ) {
	const int vowel_pause = tr->langopts.vowel_pause;
	if ( vowel_pause && ph->type != phPAUSE ) {
		if ( ph->type != phVOWEL && ( vowel_pause & 0x200 ) ) {
			insert_ph = phonPAUSE_NOLINK;
		} // after a word ending in a consonant

		if ( next->type == phVOWEL ) {
			if ( const int x = vowel_pause & 0x0c ) { // before a word starting with a vowel
				insert_ph = x == 0xc ? phonPAUSE_NOLINK : phonPAUSE_VSHORT;
			}
			if ( ph->type == phVOWEL ) {
				if ( const int x = vowel_pause & 0x03 ) {
					insert_ph = x == 2 ? phonPAUSE_SHORT : phonPAUSE_VSHORT;
				} // adjacent vowels
			}
			if ( ( plist3 + 1 )->stresslevel >= 4 && ( vowel_pause & 0x100 ) ) {
				insert_ph = phonPAUSE_SHORT;
			} // before a stressed vowel
		}
	}

	if ( !is_inserted && ix > 0 ) {
		if ( const int x = tr->langopts.word_gap & 0x7 ) {
			// a word gap never shortens an existing pause
			if ( x > 1 || ( insert_ph != phonPAUSE_SHORT && insert_ph != phonPAUSE_NOLINK ) ) {
				insert_ph = GAP_PAUSE[x];
			}
		}
	}
	return insert_ph;
}


void Engine::MakePhonemeList( Translator* tr, int post_pause, bool start_sentence ) {
	PHONEME_LIST2* plist2 = ph_list2;
	PHONEME_LIST* phlist = phoneme_list;
	const int end_sourceix = plist2[n_ph_list2 - 1].sourceix;

	promote_stress( plist2, n_ph_list2 );
	const int current_table = drop_redundant_switches( tr, plist2 );
	SelectPhonemeTable( current_table );

	if ( const int regression = tr->langopts.param[LOPT_REGRESSIVE_VOICING] ) {
		set_regressive_voicing( regression, plist2, tr );
	}

	SelectPhonemeTable( tr->phoneme_tab_ix );
	PHONEME_LIST ph_list3[N_PHONEME_LIST];
	const int n_ph_list3 = substitute_phonemes( ph_list3 ) - 2; // (the two terminating pauses are not walked)
	const int word_stress = mark_word_stress( ph_list3, n_ph_list3 );

	// transfer all the phonemes of the clause into phoneme_list
	PHONEME_TAB* ph = phoneme_of( phonPAUSE );
	ph_list3[0].ph = ph;
	int word_start = 1;
	int ix = 0;
	int insert_ph = 0;
	int unstress_count = 0;
	bool start_of_clause = true;
	PHONEME_LIST* plist3_inserted = nullptr;
	PHONEME_DATA phdata;
	WORD_PH_DATA worddata;
	std::memset( &worddata, 0, sizeof( worddata ) );
	PHONEME_TAB* next;

	SelectPhonemeTable( tr->phoneme_tab_ix );
	for ( int j = 0; insert_ph || ( j < n_ph_list3 && ix < MAX_ENTRIES ); ++j ) {
		PHONEME_LIST* plist3 = &ph_list3[j];
		bool inserted = false;
		bool deleted = false;

		if ( insert_ph != 0 ) {
			// a (linking) phoneme to insert here
			next = phoneme_of( plist3->phcode ); // this phoneme, i.e. the one after the insert

			// reuse the previous entry for the inserted phoneme. Nothing looks back from plist3 -- except CountVowelPosition() and isAfterStress
			--j;
			plist3 = plist3_inserted = &ph_list3[j];
			if ( j > 0 ) {
				// move the earlier phonemes of the word back one place
				int k;
				if ( word_start > 0 ) {
					k = word_start;
					--word_start;
				} else {
					k = 2; // no space: don't lose the start-of-word mark at ph_list2[word_start]
				}
				for ( ; k <= j; ++k ) {
					std::memcpy( &ph_list3[k - 1], &ph_list3[k], sizeof( *plist3 ) );
				}
			}
			std::memset( &plist3[0], 0, sizeof( *plist3 ) );
			plist3->phcode = insert_ph;
			ph = phoneme_of( insert_ph );
			plist3->ph = ph;
			insert_ph = 0;
			inserted = true; // don't insert the same phoneme repeatedly
		} else {
			if ( plist3->sourceix != 0 ) {
				word_start = j;
			}
			ph = phoneme_of( plist3->phcode );
			plist3[0].ph = ph;
			if ( plist3->phcode == phonSWITCH ) {
				SelectPhonemeTable( plist3->tone_ph );
			}
			next = phoneme_of( plist3[1].phcode ); // the phoneme after this one
			plist3[1].ph = next;
		}

		if ( ph == nullptr ) {
			continue;
		}

		InterpretPhoneme( tr, 0x100, plist3, &phdata, &worddata );

		if ( const int alternative = phdata.pd_param[pd_CHANGE_NEXTPHONEME]; alternative > 0 ) {
			ph_list3[j + 1].ph = phoneme_of( alternative );
			ph_list3[j + 1].phcode = alternative;
			ph_list3[j + 1].type = phoneme_of( alternative )->type;
			next = phoneme_of( alternative );
		}

		if ( const int alternative = phdata.pd_param[pd_INSERTPHONEME]; alternative > 0 && !inserted ) {
			// (upstream notes: inserting a phoneme before a vowel loses its stress)
			PHONEME_TAB* old_ph = ph;
			insert_ph = plist3->phcode;
			ph = phoneme_of( alternative );
			plist3->ph = ph;
			plist3->phcode = alternative;
			reinterpret( ph, old_ph, plist3, tr, &phdata, &worddata );
		}

		if ( const int alternative = phdata.pd_param[pd_CHANGEPHONEME]; alternative > 0 ) {
			PHONEME_TAB* old_ph = ph;
			ph = phoneme_of( alternative );
			plist3->ph = ph;
			plist3->phcode = alternative;
			if ( alternative == 1 ) {
				deleted = true; // the nullptr phoneme: discard
			} else {
				reinterpret( ph, old_ph, plist3, tr, &phdata, &worddata );
			}
		}

		if ( ph->type == phVOWEL && !deleted ) {
			reduce_unstressed( tr, plist3, word_stress, unstress_count );
		}

		if ( ( plist3 + 1 )->synthflags & SFLAG_LENGTHEN ) {
			static constexpr char TYPES_DOUBLE[] = { phFRICATIVE, phVFRICATIVE, phNASAL, phLIQUID, 0 };
			if ( j > 0 && std::strchr( TYPES_DOUBLE, next->type ) ) {
				// lengthen this consonant by doubling it (not at plist3[0]: it would crash PrevPh())
				insert_ph = next->code;
				( plist3 + 1 )->synthflags ^= SFLAG_LENGTHEN;
			}
		}

		if ( ( plist3 + 1 )->sourceix != 0 ) {
			insert_ph = boundary_pause( tr, insert_ph, ph, next, plist3, plist3 == plist3_inserted, ix );
		}

		plist3[2].ph = phoneme_of( plist3[2].phcode );

		if ( insert_ph == 0 && phdata.pd_param[pd_APPENDPHONEME] != 0 ) {
			insert_ph = phdata.pd_param[pd_APPENDPHONEME];
		}

		if ( !deleted ) {
			phlist[ix].ph = ph;
			phlist[ix].type = ph->type;
			phlist[ix].synthflags = plist3->synthflags;
			phlist[ix].stresslevel = plist3->stresslevel & 0xf;
			phlist[ix].wordstress = plist3->wordstress;
			phlist[ix].tone_ph = plist3->tone_ph;
			phlist[ix].sourceix = 0;
			phlist[ix].phcode = ph->code;
			phlist[ix].newword = 0;

			if ( plist3->sourceix != 0 ) {
				phlist[ix].sourceix = plist3->sourceix;
				phlist[ix].newword = PHLIST_START_OF_WORD;
				if ( start_sentence ) {
					phlist[ix].newword |= PHLIST_START_OF_SENTENCE;
					start_sentence = false;
				}
				if ( start_of_clause ) {
					phlist[ix].newword |= PHLIST_START_OF_CLAUSE;
					start_of_clause = false;
				}
			}
			++ix;
		}
	}

	// terminate with two pauses; the first carries the clause end
	phlist[ix].newword = PHLIST_END_OF_CLAUSE;
	phlist[ix].phcode = phonPAUSE;
	phlist[ix].type = phPAUSE;
	phlist[ix].sourceix = end_sourceix;
	phlist[ix].synthflags = 0;
	phlist[ix++].ph = phoneme_of( phonPAUSE );

	phlist[ix].newword = 0;
	phlist[ix].phcode = phonPAUSE;
	phlist[ix].type = phPAUSE;
	phlist[ix].sourceix = 0;
	phlist[ix].synthflags = 0;
	phlist[ix++].ph = phoneme_of( phonPAUSE_SHORT );

	n_phoneme_list = ix;
	(void)post_pause; // the first pause's length (by punctuation) only matters to the synthesizer

	SelectPhonemeTable( tr->phoneme_tab_ix );
}

} // namespace propan2ol
