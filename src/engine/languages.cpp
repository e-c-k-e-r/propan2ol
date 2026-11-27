// Translator creation, from espeak-ng's tr_languages.c (NewTranslator, SelectTranslator, AlphabetFromChar, the letter-group setup).
//
// A Translator holds one language's options for the dictionary, rules, stress and number code: letter groups, the stress rule,
// number syntax, and so on. Only the languages whose compiled data is embedded (en, fr, de, ja, ko, cmn) have their own settings;
// any other name gets the defaults, as in upstream's `default:` case, which only matters for a language whose dictionary isn't
// here (its load fails and the translator is dropped). Not carried over: the synthesis-only settings (stress lengths and
// amplitudes, tunes, the punctuation-to-tone table, length modifiers) and the other ~80 languages' blocks.


// start of unicode pages for character sets

#include "engine.h"

namespace propan2ol {

constexpr int OFFSET_GREEK = 0x380;
constexpr int OFFSET_CYRILLIC = 0x420;
constexpr int OFFSET_ARMENIAN = 0x530;
constexpr int OFFSET_HEBREW = 0x590;
constexpr int OFFSET_ARABIC = 0x600;
constexpr int OFFSET_SYRIAC = 0x700;
constexpr int OFFSET_DEVANAGARI = 0x900;
constexpr int OFFSET_BENGALI = 0x980;
constexpr int OFFSET_GURMUKHI = 0xa00;
constexpr int OFFSET_GUJARATI = 0xa80;
constexpr int OFFSET_ORIYA = 0xb00;
constexpr int OFFSET_TAMIL = 0xb80;
constexpr int OFFSET_TELUGU = 0xc00;
constexpr int OFFSET_KANNADA = 0xc80;
constexpr int OFFSET_MALAYALAM = 0xd00;
constexpr int OFFSET_SINHALA = 0x0d80;
constexpr int OFFSET_THAI = 0x0e00;
constexpr int OFFSET_LAO = 0x0e80;
constexpr int OFFSET_TIBET = 0x0f00;
constexpr int OFFSET_MYANMAR = 0x1000;
constexpr int OFFSET_GEORGIAN = 0x10a0;
constexpr int OFFSET_KOREAN = 0x1100;
constexpr int OFFSET_ETHIOPIC = 0x1200;
constexpr int OFFSET_SHAVIAN = 0x10450;

// character ranges must be listed in ascending unicode order
const ALPHABET alphabets[] = { { "_el", OFFSET_GREEK, 0x380, 0x3ff, L( 'e', 'l' ), AL_DONT_NAME | AL_NOT_LETTERS | AL_WORDS },
							   { "_cyr", OFFSET_CYRILLIC, 0x400, 0x52f, 0, 0 },
							   { "_hy", OFFSET_ARMENIAN, 0x530, 0x58f, L( 'h', 'y' ), AL_WORDS },
							   { "_he", OFFSET_HEBREW, 0x590, 0x5ff, 0, 0 },
							   { "_ar", OFFSET_ARABIC, 0x600, 0x6ff, 0, 0 },
							   { "_syc", OFFSET_SYRIAC, 0x700, 0x74f, 0, 0 },
							   { "_hi", OFFSET_DEVANAGARI, 0x900, 0x97f, L( 'h', 'i' ), AL_WORDS },
							   { "_bn", OFFSET_BENGALI, 0x0980, 0x9ff, L( 'b', 'n' ), AL_WORDS },
							   { "_gur", OFFSET_GURMUKHI, 0xa00, 0xa7f, L( 'p', 'a' ), AL_WORDS },
							   { "_gu", OFFSET_GUJARATI, 0xa80, 0xaff, L( 'g', 'u' ), AL_WORDS },
							   { "_or", OFFSET_ORIYA, 0xb00, 0xb7f, 0, 0 },
							   { "_ta", OFFSET_TAMIL, 0xb80, 0xbff, L( 't', 'a' ), AL_WORDS },
							   { "_te", OFFSET_TELUGU, 0xc00, 0xc7f, L( 't', 'e' ), 0 },
							   { "_kn", OFFSET_KANNADA, 0xc80, 0xcff, L( 'k', 'n' ), AL_WORDS },
							   { "_ml", OFFSET_MALAYALAM, 0xd00, 0xd7f, L( 'm', 'l' ), AL_WORDS },
							   { "_si", OFFSET_SINHALA, 0xd80, 0xdff, L( 's', 'i' ), AL_WORDS },
							   { "_th", OFFSET_THAI, 0xe00, 0xe7f, 0, 0 },
							   { "_lo", OFFSET_LAO, 0xe80, 0xeff, 0, 0 },
							   { "_ti", OFFSET_TIBET, 0xf00, 0xfff, 0, 0 },
							   { "_my", OFFSET_MYANMAR, 0x1000, 0x109f, 0, 0 },
							   { "_ka", OFFSET_GEORGIAN, 0x10a0, 0x10ff, L( 'k', 'a' ), AL_WORDS },
							   { "_ko", OFFSET_KOREAN, 0x1100, 0x11ff, L( 'k', 'o' ), AL_WORDS },
							   { "_eth", OFFSET_ETHIOPIC, 0x1200, 0x139f, 0, 0 },
							   { "_braille", 0x2800, 0x2800, 0x28ff, 0, AL_NO_SYMBOL },
							   { "_ja", 0x3040, 0x3040, 0x30ff, 0, AL_NOT_CODE },
							   { "_zh", 0x3100, 0x3100, 0x9fff, 0, AL_NOT_CODE },
							   { "_ko", 0xa700, 0xa700, 0xd7ff, L( 'k', 'o' ), AL_NOT_CODE | AL_WORDS },
							   { "_shaw", OFFSET_SHAVIAN, 0x10450, 0x1047F, L( 'e', 'n' ), 0 },
							   { nullptr, 0, 0, 0, 0, 0 } };


void Engine::set_letter_vowel( Translator* tr, int c ) {
	tr->letter_bits[c] = ( tr->letter_bits[c] & 0x40 ) | 0x81; // keep the front-vowel bit (group 6: e, i, y)
}

void Engine::set_letter_bits( Translator* tr, int group, const char* string ) {
	const int bits = 1 << group;
	for ( unsigned char c; ( c = *string++ ) != 0; ) {
		tr->letter_bits[c] |= bits;
	}
}

void Engine::set_letter_bits_range( Translator* tr, int group, int first, int last ) {
	const int bits = 1 << group;
	for ( int ix = first; ix <= last; ++ix ) {
		tr->letter_bits[ix] |= bits;
	}
}

// ignore these characters
static const unsigned short chars_ignore_default[] = {
	// U+00AD SOFT HYPHEN
	//     Used to mark hyphenation points in words for where to split a
	//     word at the end of a line to provide readable justified text.
	0xad, 1,
	// U+200C ZERO WIDTH NON-JOINER
	//     Used to prevent combined ligatures being displayed in their
	//     combined form.
	0x200c, 1,
	// U+200D ZERO WIDTH JOINER
	//     Used to indicate an alternative connected form made up of the
	//     characters surrounding the ZWJ in Devanagari, Kannada, Malayalam
	//     and Emoji.
	//	0x200d, 1, // Not ignored.
	// End of the ignored character list.
	0, 0 };

// alternatively, ignore characters but allow zero-width-non-joiner (lang-fa)
static const unsigned short chars_ignore_zwnj_hyphen[] = {
	// U+00AD SOFT HYPHEN
	//     Used to mark hyphenation points in words for where to split a
	//     word at the end of a line to provide readable justified text.
	0xad, 1,
	// U+0640 TATWEEL (KASHIDA)
	//     Used in Arabic scripts to stretch characters for justifying
	//     the text.
	0x640, 1,
	// U+200C ZERO WIDTH NON-JOINER
	//     Used to prevent combined ligatures being displayed in their
	//     combined form.
	0x200c, '-',
	// U+200D ZERO WIDTH JOINER
	//     Used to indicate an alternative connected form made up of the
	//     characters surrounding the ZWJ in Devanagari, Kannada, Malayalam
	//     and Emoji.
	//	0x200d, 1, // Not ignored.
	// End of the ignored character list.
	0, 0 };


const unsigned char UTF8_NULL[] = { 0 };

// Maps character codes transpose_min to transpose_max to 1-63 (0: no translation). Used up to 57.
const char TRANSPOSE_MAP_LATIN[] = {
	0,	1,	2,	3,	4,	5,	6,	7,	8,	9,	10, 11, 12, 13, 14, 15, // 0x60
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 0,	0,	0,	0,	0,	// 0x70
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 0x80
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 0x90
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 0xa0
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 0xb0
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 0xc0
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 0xd0
	27, 28, 29, 0,	0,	30, 31, 32, 33, 34, 35, 36, 0,	37, 38, 0,	// 0xe0
	0,	0,	0,	39, 0,	0,	40, 0,	41, 0,	42, 0,	43, 0,	0,	0,	// 0xf0
	0,	0,	0,	44, 0,	45, 0,	46, 0,	0,	0,	0,	0,	47, 0,	0,	// 0x100
	0,	48, 0,	0,	0,	0,	0,	0,	0,	49, 0,	0,	0,	0,	0,	0,	// 0x110
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 0x120
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 0x130
	0,	0,	50, 0,	51, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 0x140
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	52, 0,	0,	0,	0,	// 0x150
	0,	53, 0,	54, 0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	// 0x160
	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	55, 0,	56, 0,	57, 0,	// 0x170
};


// The options every language starts from.
Translator* Engine::new_translator() {
	static const wchar_t empty_wstring[1] = { 0 };
	static const wchar_t punct_in_word[2] = { '\'', 0 }; // an apostrophe within a word

	Translator* tr = static_cast<Translator*>( std::calloc( 1, sizeof( Translator ) ) );
	if ( tr == nullptr ) {
		return nullptr;
	}

	dictionary_name[0] = 0;
	tr->transpose_min = 0x60;
	tr->transpose_max = 0x17f;
	tr->transpose_map = TRANSPOSE_MAP_LATIN;

	// letter groups matched by A B C H F G Y and the vowel group 7 in the pronunciation rules (lower case only)
	set_letter_bits( tr, 0, "aeiou" );				  // A: vowels except y
	set_letter_bits( tr, 1, "bcdfgjklmnpqstvxz" );	  // B: hard consonants, excluding h, r, w
	set_letter_bits( tr, 2, "bcdfghjklmnpqrstvwxz" ); // C: all consonants
	set_letter_bits( tr, 3, "hlmnr" );				  // H: 'soft' consonants
	set_letter_bits( tr, 4, "cfhkpqstx" );			  // F: voiceless consonants
	set_letter_bits( tr, 5, "bdgjlmnrvwyz" );		  // G: voiced
	set_letter_bits( tr, 6, "eiy" );				  // Y: front vowels
	set_letter_bits( tr, 7, "aeiouy" );				  // vowels, including y

	tr->char_plus_apostrophe = empty_wstring;
	tr->punct_within_word = punct_in_word;
	tr->chars_ignore = chars_ignore_default;

	LANGUAGE_OPTIONS& o = tr->langopts;
	o.stress_rule = STRESSPOSN_2R;
	o.unstressed_wd1 = 1;
	o.unstressed_wd2 = 3;
	o.param[LOPT_SONORANT_MIN] = 95;
	o.param[LOPT_LONG_VOWEL_THRESHOLD] = 190 / 2;
	o.param[LOPT_MAXAMP_EOC] = 19;
	o.param[LOPT_UNPRONOUNCABLE] = 's';		   // don't count this character at the start of a word
	o.param[LOPT_BRACKET_PAUSE] = 4;		   // pause at a bracket
	o.param[LOPT_BRACKET_PAUSE_ANNOUNCED] = 2; // and when announcing its name
	o.max_initial_consonants = 3;
	o.alt_alphabet_lang = L( 'e', 'n' );
	o.roman_suffix = UTF8_NULL;
	o.max_roman = 49;
	o.min_roman = 2;
	o.thousands_sep = ',';
	o.decimal_sep = '.';
	o.numbers = NUM_DEFAULT;
	o.break_numbers = BREAK_THOUSANDS;
	o.max_digits = 14;
	return tr;
}

void Engine::setup_german( LANGUAGE_OPTIONS& o, Translator* tr ) {
	o.stress_rule = STRESSPOSN_1L;
	o.word_gap = 0x8; // no linking phonemes
	o.vowel_pause = 0x30;
	o.param[LOPT_PREFIXES] = 1;
	o.param[LOPT_REGRESSIVE_VOICING] = 0x100; // devoice at the end of a word
	o.param[LOPT_LONG_VOWEL_THRESHOLD] = 175 / 2;
	o.numbers = NUM_DECIMAL_COMMA | NUM_SWAP_TENS | NUM_ALLOW_SPACE | NUM_ORDINAL_DOT | NUM_ROMAN;
	set_letter_vowel( tr, 'y' );
	o.param[LOPT_UNPRONOUNCABLE] = 2; // de_rules decide what is unpronounceable
}

void Engine::setup_english( LANGUAGE_OPTIONS& o, Translator* tr ) {
	o.stress_rule = STRESSPOSN_1L;
	o.stress_flags = 0x08;
	o.numbers = NUM_HUNDRED_AND | NUM_ROMAN | NUM_1900;
	o.max_digits = 33;
	o.param[LOPT_COMBINE_WORDS] = 2; // "mc" combines with the following word
	o.suffix_add_e = 'e';
	o.param[LOPT_UNPRONOUNCABLE] = 2;	// en_rules decide what is unpronounceable
	set_letter_bits( tr, 6, "aeiouy" ); // group Y: vowels, including y
}

void Engine::setup_french( LANGUAGE_OPTIONS& o, Translator* tr ) {
	o.stress_rule = STRESSPOSN_1R;				// stress on the final syllable
	o.stress_flags = S_NO_AUTO_2 | S_FINAL_DIM; // no secondary stress
	o.param[LOPT_IT_LENGTHEN] = 1;				// remove the lengthen mark from unstressed syllables
	o.accents = 2;								// say "Capital" after the letter
	o.numbers = NUM_SINGLE_STRESS | NUM_DECIMAL_COMMA | NUM_ALLOW_SPACE | NUM_OMIT_1_HUNDRED | NUM_NOPAUSE | NUM_ROMAN | NUM_ROMAN_CAPITALS |
				NUM_ROMAN_AFTER | NUM_VIGESIMAL | NUM_DFRACTION_4;
	set_letter_vowel( tr, 'y' );
}

void Engine::setup_korean( LANGUAGE_OPTIONS& o, Translator* tr ) {
	static const char IVOWELS[] = { 0x63, 0x64, 0x67, 0x68, 0x6d, 0x72, 0x74, 0x75, 0 }; // y and i vowels
	static const unsigned char VOICED[] = { 0x02,
											0x05,
											0x06,
											static_cast<unsigned char>( 0xab ),
											static_cast<unsigned char>( 0xaf ),
											static_cast<unsigned char>( 0xb7 ),
											static_cast<unsigned char>( 0xbc ),
											0 }; // voiced consonants l, m, n, N

	tr->letter_bits_offset = OFFSET_KOREAN;
	o.our_alphabet = 0xa700;
	std::memset( tr->letter_bits, 0, sizeof( tr->letter_bits ) );
	set_letter_bits_range( tr, LETTERGP_A, 0x61, 0x75 );
	set_letter_bits( tr, LETTERGP_Y, IVOWELS );
	set_letter_bits( tr, LETTERGP_G, reinterpret_cast<const char*>( VOICED ) );

	o.stress_rule = STRESSPOSN_2LLH;  // the first syllable if heavy, else the second
	o.param[LOPT_UNPRONOUNCABLE] = 1; // no unpronounceable-word check
	o.numbers = NUM_OMIT_1_HUNDRED;
	o.numbers2 = NUM2_MYRIADS;
	o.break_numbers = BREAK_MYRIADS;
	o.max_digits = 20;
}

void Engine::setup_chinese( LANGUAGE_OPTIONS& o, bool cantonese ) {
	o.stress_rule = STRESSPOSN_1R; // stress on the final syllable of a "word"
	o.stress_flags = S_NO_DIM;	   // no automatic diminished stress
	o.vowel_pause = 0;
	o.tone_language = 1;
	o.tone_numbers = 1; // a number after letters is a tone number (pinyin, jyutping)
	o.ideographs = 1;
	o.our_alphabet = 0x3100;
	o.word_gap = 0x21; // a final vowel's length depends less on the next consonant; don't merge a consonant with the next word
	o.textmode = true;
	o.listx = 1; // *_listx is compiled after *_list
	if ( cantonese ) {
		o.numbers = NUM_DEFAULT;
		o.numbers2 = NUM2_ZERO_TENS;
		o.break_numbers = BREAK_INDIVIDUAL;
	}
}


// The alphabet a character belongs to, or nullptr.
const ALPHABET* Engine::AlphabetFromChar( int c ) {
	for ( const ALPHABET* alphabet = alphabets; alphabet->name != nullptr; ++alphabet ) {
		if ( c <= static_cast<int>( alphabet->range_max ) ) {
			return c >= static_cast<int>( alphabet->range_min ) ? alphabet : nullptr;
		}
	}
	return nullptr;
}

// Folds the number-separator settings that follow from the number syntax flags.
void Engine::ProcessLanguageOptions( LANGUAGE_OPTIONS* langopts ) {
	if ( langopts->numbers & NUM_DECIMAL_COMMA ) { // '.' for thousands, ',' for decimals
		langopts->thousands_sep = '.';
		langopts->decimal_sep = ',';
	}
	if ( langopts->numbers & NUM_THOUS_SPACE ) {
		langopts->thousands_sep = 0;
	} // only a space separates thousands
}

Translator* Engine::SelectTranslator( const char* name ) {
	Translator* tr = new_translator();
	if ( tr == nullptr ) {
		return nullptr;
	}
	std::strcpy( tr->dictionary_name, name );

	// the name (up to 4 characters) as one integer
	int name2 = 0;
	for ( const char* p = name; *p != 0; ++p ) {
		name2 = ( name2 << 8 ) + *p;
	}

	switch ( name2 ) {
	case L( 'd', 'e' ): setup_german( tr->langopts, tr ); break;
	case L( 'e', 'n' ): setup_english( tr->langopts, tr ); break;
	case L( 'f', 'r' ): setup_french( tr->langopts, tr ); break;
	case L( 'k', 'o' ): setup_korean( tr->langopts, tr ); break;
	case L3( 'c', 'm', 'n' ):
	case L( 'z', 'h' ): setup_chinese( tr->langopts, false ); break;
	case L3( 'y', 'u', 'e' ): setup_chinese( tr->langopts, true ); break;
	default: tr->langopts.param[LOPT_UNPRONOUNCABLE] = 1; break; // Japanese and the rest: no unpronounceable-word check
	}

	tr->translator_name = name2;
	ProcessLanguageOptions( &tr->langopts );
	return tr;
}

} // namespace propan2ol
