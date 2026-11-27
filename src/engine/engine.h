// The engine: one language's text to phonemes, with all its state in the object (nothing is global or shared, so engines are independent).
//
// Its methods are espeak-ng's translation code, from dictionary.c, numbers.c, translate.c, translateword.c, readclause.c, phonemelist.c,
// synthdata.c and tr_languages.c, in the files of src/engine/ named after them; the names are upstream's.
#pragma once

#include "types.h"
#include "../chars.h"
#include "../text.h"
#include "../text_decoder.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <algorithm>
#include <climits>
#include <cctype>
#include <cstring>
#include <memory>
#include <optional>

namespace propan2ol {

// espeak-ng's common.c helpers, over src/text.h and src/chars.h
inline int utf8_in( int* c, const char* buf ) { return text::utf8_in( *c, buf, false ); }
inline int utf8_in2( int* c, const char* buf, int backwards ) { return text::utf8_in( *c, buf, backwards != 0 ); }
inline int utf8_out( unsigned c, char* buf ) { return text::utf8_out( c, buf ); }
inline int IsAlpha( unsigned c ) { return text::is_alpha( c ); }
inline int IsBracket( int c ) { return text::is_bracket( c ); }
inline int IsDigit09( unsigned c ) { return text::is_digit09( c ); }
inline int IsDigit( unsigned c ) { return text::is_digit( c ); }
inline int IsSpace( unsigned c ) { return text::is_space( c ); }
inline int isspace2( unsigned c ) { return text::is_space_ascii( c ); }
inline int is_str_totally_null( const char* str, int size ) { return text::is_all_zero( str, size ); }
inline void strncpy0( char* to, const char* from, int size ) { std::snprintf( to, size, "%s", from ); }
inline int towlower2( unsigned c, const Translator* tr ) {
	return c == 'I' && tr->langopts.dotless_i ? 0x131 : chars::ucd_to_lower( c );
} // Turkish I

extern const char stress_phonemes[];

// strcat into an array, truncating rather than overflowing it
template <size_t N> inline void append( char ( &dst )[N], const char* src ) {
	const size_t used = std::strlen( dst );
	const size_t add = std::min( std::strlen( src ), N - 1 - used );
	std::memmove( dst + used, src, add );
	dst[used + add] = 0;
}

// snprintf into an array, bounded by its size (a pointer is a compile error)
template <size_t N, class... Args> inline int format( char ( &dst )[N], const char* fmt, Args... args ) {
	return std::snprintf( dst, N, fmt, args... );
}

class Engine {
  public:
	Engine() = default;
	~Engine();
	Engine( const Engine& ) = delete;
	Engine& operator=( const Engine& ) = delete;

	// nullptr if the embedded data is bad
	static std::unique_ptr<Engine> create();
	// the embedded languages' codes, null-terminated
	static const char* const* languages();
	// false if the language isn't one of the embedded ones
	bool set_language( const char* code );
	// espeak_TextToPhonemes: one clause per call, advancing *textptr (nullptr at the end). `phonememode` as espeak's: bit 1 IPA, bit 7 the tie
	// in bits 8-23 joins a phoneme's characters, else bits 8-23 are the separator between phonemes.
	const char* text_to_phonemes( const void** textptr, int phonememode );

  private:
	// The state of scanning one clause's text into words (TranslateClauseWithTerminator): the normalised text, the words found, and the
	// characters around the one being read.
	struct ClauseScan {
		short charix
			[N_TR_SOURCE +
			 4]; // for each byte of the clause's text: the character's position in the original text (-1: a later byte of a multibyte character)
		int charix_top = 0;
		char sbuf[N_TR_SOURCE]; // the normalised text, each word followed by a space
		int ix = 0;				// the end of the text in sbuf
		WORD_TAB words[N_CLAUSE_WORDS];
		int word_count = 0;
		int terminator = 0;	  // how the clause ends, as ReadClause returns it
		int clause_pause = 0; // ms
		bool new_sentence2 = false;
		unsigned int source_index = 0; // the read position in `source`
		int source_index_word = 0;	   // where the current word began in `source`
		unsigned int prev_source_index = 0;
		int c = 0;	// the current character, as changed
		int cc = 0; // as read
		int prev_in = ' ', prev_out = ' ', prev_out2 = 0, prev_in_save = 0, next_in = 0, next_in_nbytes = 0;
		int char_inserted = 0;
		int pre_pause_add = 0;
		int all_upper_case = FLAG_ALL_UPPER;
		int alpha_count = 0;
		int letter_count = 0;
		bool finished = false;
		bool single_quoted = false;
		bool phoneme_mode = false;
		bool space_inserted = false;
		bool syllable_marked = false;
		bool decimal_sep_count = false;
		int word_flags = 0;
		int next_word_flags = 0;
	};

	// One word being translated (TranslateWord3): its arguments, the pieces of its pronunciation, and what has been learned about it.
	struct WordParse {
		char* word_start = nullptr;
		WORD_TAB* wtab = nullptr;
		char* word_out = nullptr;
		bool* any_stressed_words_arg = nullptr;
		ALPHABET* current_alphabet_arg = nullptr;
		char* word_phonemes_arg = nullptr;
		size_t size_word_phonemes = 0;
		WORD_TAB wtab_null[8]; // stands in for wtab if there is none

		char* word1 = nullptr; // the word (after a space that replaced a dot)
		char* wordx = nullptr; // the part of the word still to translate
		int word_length = 0;   // in characters
		int first_char = 0;
		int last_char = 0;
		char word_copy2[N_WORD_BYTES]; // the word as it was, to restore it
		int word_copy_length = 0;
		int wflags = 0;
		int spell_word = 0;
		bool found = false;
		int was_unpronouncable = 0;
		int c_temp = 0; // a character byte that is temporarily replaced by a space

		unsigned int dictionary_flags[2] = { 0, 0 };
		unsigned int dictionary_flags2[2] = { 0, 0 };
		char phonemes[N_WORD_PHONEMES];
		char phonemes2[N_WORD_PHONEMES];
		char unpron_phonemes[N_WORD_PHONEMES]; // the letters spoken individually before the pronounceable part
		char prefix_phonemes[N_WORD_PHONEMES];
		char end_phonemes[N_WORD_PHONEMES];
		char end_phonemes2[N_WORD_PHONEMES];
		char word_copy[N_WORD_BYTES];
		char prefix_chars[0x3f + 2];
		int end_type = 0;
		int end_type1 = 0;
		int prefix_type = 0;
		int prefix_flags = 0;
	};

	// The state of matching one letter-to-sound rule against the word (MatchRule): where the rule and the input are, and the points so far.
	struct RuleScan {
		char** word = nullptr; // the current character group in the word; advanced as characters are consumed
		char* word_start = nullptr;
		int group_length = 0;
		int word_flags = 0;
		int dict_flags = 0;
		char* rule = nullptr; // the next byte of the rule
		MatchRecord match;
		char* common_phonemes = nullptr; // common to a group of entries
		bool check_atstart = false;
		int consumed = 0;
		int distance_left = 0;
		int distance_right = 0;
		int failed = 0; // 1: no match; 2: matched
		int unpron_ignore = 0;
		int match_type = 0; // 0: consume, RULE_PRE: left, RULE_POST: right
		char* pre_ptr = nullptr;
		char* post_ptr = nullptr; // the first character after the group
		unsigned char rb = 0;	  // the current instruction of the rule
		unsigned char letter = 0; // the current input letter, one byte
		int letter_w = 0;		  // the same, as a wide character
		int last_letter_w = 0;
		int letter_xbytes = 0; // the extra bytes of a multibyte character
		int add_points = 0;
		char word_buf[N_WORD_BYTES];
	};

	// The state of guessing a word's stress pattern (SetWordStress): its phonemes, and per vowel its stress, weight and length (index 1 is the
	// first vowel).
	struct StressScan {
		unsigned char phonetic[N_WORD_PHONEMES];
		signed char vowel_stress[N_WORD_PHONEMES / 2];
		char syllable_weight[N_WORD_PHONEMES / 2];
		char vowel_length[N_WORD_PHONEMES / 2];
		int vowel_count = 0; // the number of vowels + 1
		int stressed_syllable = 0;
		int max_stress = 0;
		int max_stress_input = 0; // any stress specified in the input?
		int max_stress_posn = 0;
		bool unstressed_word = false;
		int final_ph = 0;
		int final_ph2 = 0;
		int stressflags = 0;
	};

	// What TranslateWord2 knows about a word while it turns it into phonemes.
	struct WordPhonemes {
		int flags = 0;			  // the word's dictionary flags
		int switch_phonemes = -1; // the phoneme table of another language the word is in, or -1
		char old_dictionary_name[40];
		unsigned char* p = nullptr; // the word's phoneme codes (in word_phonemes)
		int word_flags = 0;
		int source_ix = 0; // the word's position and length in the source text
	};

	// ---- the state: espeak-ng's file-scope variables, per engine ----
	// the language and its translators (the second and third are for words switched to another language)
	Translator* translator = nullptr;
	Translator* translator2 = nullptr;
	char translator2_language[20] = { 0 };
	Translator* translator3 = nullptr;
	char translator3_language[20] = { 0 };
	int voice_phoneme_tab_ix = 0; // the language's own phoneme table (the one to go back to after a switch)
	int n_replace_phonemes;
	REPLACE_PHONEMES replace_phonemes[N_REPLACE_PHONEMES];

	// phoneme tables (read in place from the embedded data) and the table in use
	const unsigned short* phoneme_index = nullptr;
	int n_phoneme_tables = 0;
	int current_phoneme_table = -1;
	int phoneme_tab_number = 0;
	PHONEME_TAB_LIST phoneme_tab_list[N_PHONEME_TABS];
	int n_phoneme_tab;
	PHONEME_TAB* phoneme_tab[N_PHONEME_TAB];

	// the clause being read and translated
	TextDecoder decoder;
	int ungot_char;
	int ungot_char2 = 0;
	int count_characters = 0;
	int clause_start_char;
	int count_sentences;
	int count_words;
	bool new_sentence;
	int max_clause_pause = 0;
	char source[N_TR_SOURCE + 40];
	char voice_change_name[40];
	int pre_pause;
	bool any_stressed_words;
	ALPHABET* current_alphabet;
	char word_phonemes[N_WORD_PHONEMES];
	int ignore_next_n = 0;

	// the clause's phonemes: the first stage (one entry per phoneme), then the final list the IPA is written from
	int n_ph_list2;
	PHONEME_LIST2 ph_list2[N_PHONEME_LIST];
	int n_phoneme_list = 0;
	PHONEME_LIST phoneme_list[N_PHONEME_LIST + 1];

	// the dictionary and rules
	char dictionary_name[40];
	int dictionary_skipwords;
	char word_replacement[N_WORD_BYTES];
	char* phon_out_buf = nullptr;
	unsigned int phon_out_size = 0;

	// numbers
	char* digit_lookup;
	int n_digit_lookup;
	int number_control;
	char ph_ordinal2[12];
	char ph_ordinal2x[12];
	int speak_missing_thousands;

	// ---- the methods, by source file ----

	// translate.cpp: clauses and words
	void DeleteTranslator( Translator* tr );
	int lookupwchar( const unsigned short* list, int c );
	char* strchr_w( const char* s, int c );
	int TranslateWord( Translator* tr, char* word_start, WORD_TAB* wtab, char* word_out );
	void SetPlist2( PHONEME_LIST2* p, unsigned char phcode );
	int CountSyllables( unsigned char* phonemes );
	int SetAlternateTranslator( const char* new_language, Translator** translator, char translator_language[20] );
	int SetTranslator2( const char* new_language );
	int SetTranslator3( const char* new_language );
	int TranslateWord2( Translator* tr, char* word, WORD_TAB* wtab, int pre_pause );
	std::optional<int> WordToPhonemeCodes( Translator* tr, char* word, WORD_TAB* wtab, int& pre_pause, WordPhonemes& wp );
	void AddPauses( Translator* tr, int& pre_pause );
	int AddWordPhonemes( Translator* tr, WordPhonemes& wp );
	const char* FindReplacementChars( Translator* tr, const char** pfrom, unsigned int c, const char* next, int* ignore_next_n );
	int SubstituteChar( Translator* tr, unsigned int c, unsigned int next_in, const char* next, int* insert, int* wordflags );
	int TranslateChar( Translator* tr, char* ptr, int prev_in, unsigned int c, unsigned int next_in, int* insert, int* wordflags );
	int UpperCaseInWord( Translator* tr, char* word, int c );
	void TranslateClauseWithTerminator( Translator* tr, int* tone_out, char** voice_change, int* terminator_out );
	void BeginClause( Translator* tr, ClauseScan& s, int* tone_out, int* terminator_out );
	void ScanWords( Translator* tr, ClauseScan& s );
	void ReadScanChar( Translator* tr, ClauseScan& s );
	bool ScanTextChar( Translator* tr, ClauseScan& s );
	void ScanLetter( Translator* tr, ClauseScan& s );
	void ScanHyphen( Translator* tr, ClauseScan& s );
	void ScanDot( Translator* tr, ClauseScan& s );
	void ScanApostrophe( Translator* tr, ClauseScan& s );
	void ScanDigit( Translator* tr, ClauseScan& s );
	bool EndWord( Translator* tr, ClauseScan& s );
	void FinishWordList( Translator* tr, ClauseScan& s );
	void TranslateWords( Translator* tr, ClauseScan& s, int* tone_out );
	void RemoveThousandsSeparators( Translator* tr, ClauseScan& s, int& ix, char* word );
	int TranslateLongNumber( Translator* tr, ClauseScan& s, int ix, char* word, int n_digits );
	int TranslatePlainWord( Translator* tr, ClauseScan& s, int ix, char* word, int* tone_out );

	void TranslateClause( Translator* tr, int* tone_out, char** voice_change );
	int CalcWordLength( int source_index, int charix_top, short int* charix, WORD_TAB* words, int word_count );
	void CombineFlag( Translator* tr, WORD_TAB* wtab, char* word, int* flags, unsigned char* p, char* word_phonemes );
	void SwitchLanguage( char* word, char* word_phonemes );

	// translateword.cpp: one word
	int TranslateWord3( Translator* tr, char* word_start, WORD_TAB* wtab, char* word_out, bool* any_stressed_words_arg,
						ALPHABET* current_alphabet_arg, char word_phonemes_arg[], size_t size_word_phonemes );
	std::optional<int> LookUpWord( Translator* tr, WordParse& w );
	int SpellWord( Translator* tr, WordParse& w );
	std::optional<int> TranslateUnlistedWord( Translator* tr, WordParse& w );
	std::optional<int> RemovePrefixes( Translator* tr, WordParse& w );
	std::optional<int> RemoveSuffixes( Translator* tr, WordParse& w );
	int FinishWord( Translator* tr, WordParse& w );

	void ApplySpecialAttribute2( Translator* tr, char* phonemes, int dict_flags );
	void ChangeWordStress( Translator* tr, char* word, int new_stress );
	char* SpeakIndividualLetters( Translator* tr, char* word, char* phonemes, int spell_word, const ALPHABET* current_alphabet_arg,
								  char word_phonemes_arg[] );
	int TranslateLetter( Translator* tr, char* word, char* phonemes, int control, const ALPHABET* current_alphabet_arg );
	void addPluralSuffixes( int flags, Translator* tr, char last_char, char* word_phonemes_arg );
	int CheckDottedAbbrev( char* word1 );
	int NonAsciiNumber( int letter );
	int Unpronouncable( Translator* tr, char* word, int posn );
	int Unpronouncable2( Translator* tr, char* word );

	// dictionary.cpp: the dictionary, letter-to-sound rules, stress and the IPA writer
	int Reverse4Bytes( int word );
	void InitGroups( Translator* tr );
	int LoadDictionary( Translator* tr, const char* name, int no_error );
	int HashDictionary( const char* string );
	const char* EncodePhonemes( const char* p, char* outptr, int* bad_phoneme );
	void DecodePhonemes( const char* inptr, char* outptr );
	char* WritePhMnemonic( char* phon_out, PHONEME_TAB* ph, PHONEME_LIST* plist, int use_ipa, int* flags );
	const char* GetTranslatedPhonemeString( int phoneme_mode );
	int LetterGroupNo( char* rule );
	int IsLetterGroup( Translator* tr, char* word, int group, int pre );
	int IsLetter( Translator* tr, int letter, int group );
	int IsVowel( Translator* tr, int letter );
	int GetVowelStress( Translator* tr, unsigned char* phonemes, signed char* vowel_stress, int* vowel_count, int* stressed_syllable, int control );
	void SetWordStress( Translator* tr, char* output, unsigned int* dictionary_flags, int tonic, int control );
	void WeighSyllables( StressScan& st );
	void ApplyStressRule( Translator* tr, StressScan& st, int control );
	void SpreadStress( Translator* tr, StressScan& st, int tonic );
	void WriteStressedPhonemes( Translator* tr, StressScan& st, char* output, int control );

	void AppendPhonemes( Translator* tr, char* string, int size, const char* ph );
	void MatchRule( Translator* tr, char* word[], char* word_start, int group_length, char* rule, MatchRecord* match_out, int word_flags,
					int dict_flags );
	void RuleInstruction( Translator* tr, RuleScan& r );
	void MatchLetter( RuleScan& r );
	void MatchPostLetter( Translator* tr, RuleScan& r );
	void MatchPreLetter( Translator* tr, RuleScan& r );

	int TranslateRules( Translator* tr, char* p_start, char* phonemes, int ph_size, char* end_phonemes, int word_flags, unsigned int* dict_flags );
	int TransposeAlphabet( Translator* tr, char* text );
	const char* LookupDict2( Translator* tr, const char* word, const char* word2, char* phonetic, unsigned int* flags, int end_flags,
							 WORD_TAB* wtab );
	int utf8_nbytes( const char* buf );
	int LookupDictList( Translator* tr, char** wordptr, char* ph_out, unsigned int* flags, int end_flags, WORD_TAB* wtab );
	int Lookup( Translator* tr, const char* word, char* ph_out );
	int LookupFlags( Translator* tr, const char* word, unsigned int flags_out[2] );
	int RemoveEnding( Translator* tr, char* word, int end_type, char* word_copy );
	void DollarRule( char* word[], char* word_start, int consumed, int group_length, char* word_buf, Translator* tr, int command, int* failed,
					 int* add_points );

	// numbers.cpp: letters, numbers and roman numerals
	int LookupLetter2( Translator* tr, unsigned int letter, char* ph_buf );
	void LookupAccentedLetter( Translator* tr, unsigned int letter, char* ph_buf );
	void LookupLetter( Translator* tr, unsigned int letter, int next_byte, char* ph_buf1, int control );
	int IsSuperscript( int letter );
	void SetSpellingStress( Translator* tr, char* phonemes, int control, int n_chars );
	int CheckDotOrdinal( Translator* tr, char* word, char* word_end, WORD_TAB* wtab, int roman );
	int hu_number_e( const char* word, int thousandplex, int value );
	int TranslateRoman( Translator* tr, char* word, char* ph_out, WORD_TAB* wtab );
	const char* M_Variant( int value );
	int LookupThousands( Translator* tr, int value, int thousandplex, int thousands_exact, char* ph_out );
	int LookupNum2( Translator* tr, int value, int thousandplex, const int control, char* ph_out );
	int LookupNum3( Translator* tr, int value, char* ph_out, bool suppress_null, int thousandplex, int control );
	bool CheckThousandsGroup( char* word, int group_len );
	int TranslateNumber_1( Translator* tr, char* word, char* ph_out, unsigned int* flags, WORD_TAB* wtab, int control );
	void SpeakDecimals( Translator* tr, char* word, char* ph_out, int& n_digits, int& decimal_point );
	void LookupOrdinalSuffix( Translator* tr, char* word, int ix, WORD_TAB* wtab, unsigned int* flags, int& ordinal, int& skipwords );
	bool LookupNumberCombination( Translator* tr, char* word, char* buf_digit_lookup, unsigned int* flags, WORD_TAB* wtab, int prev_thousands,
								  int decimal_point, int ordinal, long value, char*& ph_out );
	int TranslateNumber( Translator* tr, char* word1, char* ph_out, unsigned int* flags, WORD_TAB* wtab, int control );

	// phoneme_data.cpp: phoneme tables and phoneme programs
	PHONEME_TAB* table_of( int phcode );
	void set_up_table( int number );
	bool stress_condition( Translator* tr, PHONEME_LIST* plist, int condition, int control );
	int count_vowel_position( PHONEME_LIST* plist );
	bool interpret_condition( Translator* tr, int control, PHONEME_LIST* plist, const unsigned short* p_prog, WORD_PH_DATA* worddata );
	int n_instn_words( const unsigned short* prog );
	bool LoadPhData();
	int PhonemeCode( unsigned int mnem );
	int LookupPhonemeString( const char* string );
	void SelectPhonemeTable( int number );
	int LookupPhonemeTable( const char* name );
	int SelectPhonemeTableName( const char* name );
	void InterpretPhoneme( Translator* tr, int control, PHONEME_LIST* plist, PHONEME_DATA* phdata, WORD_PH_DATA* worddata );
	void InterpretPhoneme2( int phcode, PHONEME_DATA* phdata );

	// phoneme_list.cpp: the clause's phoneme list
	PHONEME_TAB* phoneme_of( int phcode );
	void promote_stress( PHONEME_LIST2* plist2, int n );
	int drop_redundant_switches( Translator* tr, PHONEME_LIST2* plist2 );
	void set_regressive_voicing( int regression, PHONEME_LIST2* plist2, Translator* tr );
	int substitute_phonemes( PHONEME_LIST* out );
	int mark_word_stress( PHONEME_LIST* list, int n );
	void reinterpret( PHONEME_TAB* ph, PHONEME_TAB* old_ph, PHONEME_LIST* plist3, Translator* tr, PHONEME_DATA* phdata, WORD_PH_DATA* worddata );
	void reduce_unstressed( Translator* tr, PHONEME_LIST* plist3, int word_stress, int& unstress_count );
	int boundary_pause( Translator* tr, int insert_ph, PHONEME_TAB* ph, PHONEME_TAB* next, PHONEME_LIST* plist3, bool is_inserted, int ix );
	void MakePhonemeList( Translator* tr, int post_pause, bool start_sentence );

	// clause_reader.cpp: reading one clause
	bool at_end();
	int get_char();
	void unget_char( int c );
	bool is_roman_upper( unsigned c );
	void terminate( char* buf, int index, const int* unget );
	int replacement( const unsigned short* list, int c );
	bool ignore_or_replace( Translator* tr, int& c1 );
	void lookup_char_name( char buf[60], Translator* tr, int c );
	int clause_type_from_codepoint( uint32_t c );
	int Eof();
	const char* WordToString2( char buf[5], unsigned int word );
	int ReadClause( Translator* tr, char* buf, short* charix, int* charix_top, int n_buf, int* tone_type, char* voice_change );

	// languages.cpp: translators and alphabets
	void set_letter_vowel( Translator* tr, int c );
	void set_letter_bits( Translator* tr, int group, const char* string );
	void set_letter_bits_range( Translator* tr, int group, int first, int last );
	Translator* new_translator();
	void setup_german( LANGUAGE_OPTIONS& o, Translator* tr );
	void setup_english( LANGUAGE_OPTIONS& o, Translator* tr );
	void setup_french( LANGUAGE_OPTIONS& o, Translator* tr );
	void setup_korean( LANGUAGE_OPTIONS& o, Translator* tr );
	void setup_chinese( LANGUAGE_OPTIONS& o, bool cantonese );
	const ALPHABET* AlphabetFromChar( int c );
	void ProcessLanguageOptions( LANGUAGE_OPTIONS* langopts );
	Translator* SelectTranslator( const char* name );
};

} // namespace propan2ol
