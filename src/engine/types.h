// The engine's types and constants, from espeak-ng's phoneme.h, translate.h and synthesize.h, without what only synthesis needs.
// (The structs are laid out as upstream's: the phoneme tables are read in place from the embedded blobs.)
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

inline constexpr const char* ESPEAKNG_DEFAULT_VOICE = "en";

// phoneme types
inline constexpr int phPAUSE = 0;
inline constexpr int phSTRESS = 1;
inline constexpr int phVOWEL = 2;
inline constexpr int phLIQUID = 3;
inline constexpr int phSTOP = 4;
inline constexpr int phVSTOP = 5;
inline constexpr int phFRICATIVE = 6;
inline constexpr int phVFRICATIVE = 7;
inline constexpr int phNASAL = 8;
inline constexpr int phVIRTUAL = 9;
inline constexpr int phDELETED = 14;
inline constexpr int phINVALID = 15;

// places of articulation (phARTICULATION)
inline constexpr int phPLACE_BILABIAL = 1;
inline constexpr int phPLACE_LABIODENTAL = 2;
inline constexpr int phPLACE_DENTAL = 3;
inline constexpr int phPLACE_ALVEOLAR = 4;
inline constexpr int phPLACE_RETROFLEX = 5;
inline constexpr int phPLACE_PALATO_ALVEOLAR = 6;
inline constexpr int phPLACE_PALATAL = 7;
inline constexpr int phPLACE_VELAR = 8;
inline constexpr int phPLACE_LABIO_VELAR = 9;
inline constexpr int phPLACE_UVULAR = 10;
inline constexpr int phPLACE_PHARYNGEAL = 11;
inline constexpr int phPLACE_GLOTTAL = 12;

// phflags
inline constexpr int phFLAGBIT_UNSTRESSED = 1;
inline constexpr int phFLAGBIT_VOICELESS = 3;
inline constexpr int phFLAGBIT_VOICED = 4;
inline constexpr int phFLAGBIT_SIBILANT = 5;
inline constexpr int phFLAGBIT_NOLINK = 6;
inline constexpr int phFLAGBIT_TRILL = 7;
inline constexpr int phFLAGBIT_PALATAL = 9;
inline constexpr int phFLAGBIT_BRKAFTER = 14;	 // [*] add a post-pause
inline constexpr int phARTICULATION = 0xf0000;	 // bits 16-19
inline constexpr int phFLAGBIT_NONSYLLABIC = 20; // don't count this vowel as a syllable when finding the stress position
inline constexpr int phFLAGBIT_LONG = 21;
inline constexpr int phFLAGBIT_LENGTHENSTOP = 22; // make the pre-pause slightly longer
inline constexpr int phFLAGBIT_RHOTIC = 23;
inline constexpr int phFLAGBIT_NOPAUSE = 24;
inline constexpr int phFLAGBIT_PREVOICE = 25; // for voiced stops
inline constexpr int phFLAGBIT_FLAG1 = 28;
inline constexpr int phFLAGBIT_FLAG2 = 29;
inline constexpr int phFLAGBIT_LOCAL = 31; // used during compilation

// phoneme properties
inline constexpr unsigned phUNSTRESSED = ( 1U << phFLAGBIT_UNSTRESSED );
inline constexpr unsigned phVOICELESS = ( 1U << phFLAGBIT_VOICELESS );
inline constexpr unsigned phVOICED = ( 1U << phFLAGBIT_VOICED );
inline constexpr unsigned phSIBILANT = ( 1U << phFLAGBIT_SIBILANT );
inline constexpr unsigned phNOLINK = ( 1U << phFLAGBIT_NOLINK );
inline constexpr unsigned phTRILL = ( 1U << phFLAGBIT_TRILL );
inline constexpr unsigned phPALATAL = 1U << phFLAGBIT_PALATAL;
inline constexpr unsigned phBRKAFTER = ( 1U << phFLAGBIT_BRKAFTER );
inline constexpr unsigned phNONSYLLABIC = ( 1U << phFLAGBIT_NONSYLLABIC );
inline constexpr unsigned phLONG = ( 1U << phFLAGBIT_LONG );
inline constexpr unsigned phLENGTHENSTOP = ( 1U << phFLAGBIT_LENGTHENSTOP );
inline constexpr unsigned phRHOTIC = ( 1U << phFLAGBIT_RHOTIC );
inline constexpr unsigned phNOPAUSE = ( 1U << phFLAGBIT_NOPAUSE );
inline constexpr unsigned phPREVOICE = ( 1U << phFLAGBIT_PREVOICE );
inline constexpr unsigned phFLAG1 = ( 1U << phFLAGBIT_FLAG1 );
inline constexpr unsigned phFLAG2 = ( 1U << phFLAGBIT_FLAG2 );
inline constexpr unsigned phLOCAL = 1U << phFLAGBIT_LOCAL;

// fixed phoneme code numbers, these can be used from the program code
inline constexpr int phonCONTROL = 1;
inline constexpr int phonSTRESS_U = 2;
inline constexpr int phonSTRESS_D = 3;
inline constexpr int phonSTRESS_2 = 4;
inline constexpr int phonSTRESS_3 = 5;
inline constexpr int phonSTRESS_P = 6;
inline constexpr int phonSTRESS_P2 = 7; // priority stress within a word
inline constexpr int phonSTRESS_PREV = 8;
inline constexpr int phonPAUSE = 9;
inline constexpr int phonPAUSE_SHORT = 10;
inline constexpr int phonPAUSE_NOLINK = 11;
inline constexpr int phonLENGTHEN = 12;
inline constexpr int phonSCHWA = 13;
inline constexpr int phonSCHWA_SHORT = 14;
inline constexpr int phonEND_WORD = 15;
inline constexpr int phonDEFAULTTONE = 17;
inline constexpr int phonCAPITAL = 18;
inline constexpr int phonGLOTTALSTOP = 19;
inline constexpr int phonSYLLABIC = 20;
inline constexpr int phonSWITCH = 21;
inline constexpr int phonX1 = 22; // a language specific action
inline constexpr int phonPAUSE_VSHORT = 23;
inline constexpr int phonPAUSE_LONG = 24;
inline constexpr int phonT_REDUCED = 25;
inline constexpr int phonSTRESS_TONIC = 26;
inline constexpr int phonPAUSE_CLAUSE = 27;
inline constexpr int phonVOWELTYPES = 28; // 28 to 33

inline constexpr int N_PHONEME_TABS = 150;	  // number of phoneme tables
inline constexpr int N_PHONEME_TAB = 256;	  // max phonemes in a phoneme table
inline constexpr int N_PHONEME_TAB_NAME = 32; // must be multiple of 4

// main table of phonemes, index by phoneme number (1-254)

struct PHONEME_TAB {
	unsigned int mnemonic;	// Up to 4 characters.  The first char is in the l.s.byte
	unsigned int phflags;	// bits 16-19 place of articulation
	unsigned short program; // index into phondata file
	unsigned char code;		// the phoneme number
	unsigned char type;		// phVOWEL, phPAUSE, phSTOP etc
	unsigned char start_type;
	unsigned char end_type;	  // vowels: endtype; consonant: voicing switch
	unsigned char std_length; // for vowels, in mS/2;  for phSTRESS phonemes, this is the stress/tone type
	unsigned char length_mod; // a length_mod group number, used to access length_mod_tab
};


// Several phoneme tables may be loaded into memory. phoneme_tab points to
// one for the current voice

struct PHONEME_TAB_LIST {
	char name[N_PHONEME_TAB_NAME];
	PHONEME_TAB* phoneme_tab_ptr;
	int n_phonemes;
	int includes; // also include the phonemes from this other phoneme table
};

// table of phonemes to be replaced with different phonemes, for the current voice
inline constexpr int N_REPLACE_PHONEMES = 60;
struct REPLACE_PHONEMES {
	unsigned char old_ph;
	unsigned char new_ph;
	char type; // 0=always replace, 1=only at end of word
};


// Table of phoneme programs and lengths.  Used by MakeVowelLists
struct PHONEME_PROG_LOG {
	unsigned int addr;
	unsigned int length;
};


// two or three characters as one integer: a translator's name
constexpr int L( int c1, int c2 ) { return ( c1 << 8 ) + c2; }
constexpr int L3( int c1, int c2, int c3 ) { return ( c1 << 16 ) + ( c2 << 8 ) + c3; }

inline constexpr int CTRL_EMBEDDED = 0x01; // control character at the start of an embedded command
inline constexpr int REPLACED_E = 'E';	   // 'e' replaced by silent e

inline constexpr int N_WORD_PHONEMES = 200; // max phonemes in a word
inline constexpr int N_WORD_BYTES = 160;	// max bytes for the UTF8 characters in a word
inline constexpr int N_PHONEME_BYTES = 160; // max bytes for a phoneme
inline constexpr int N_CLAUSE_WORDS = 300;	// max words in a clause
inline constexpr int N_TR_SOURCE = 800;		// the source text of a single clause (UTF8 bytes)

inline constexpr int N_RULE_GROUP2 = 120; // max num of two-letter rule chains
inline constexpr int N_HASH_DICT = 1024;
inline constexpr int N_LETTER_GROUPS = 95; // maximum is 127-32

// dictionary flags, word 1
// bits 0-3  stressed syllable,  bit 6=unstressed
inline constexpr int FLAG_SKIPWORDS = 0x80;
inline constexpr int FLAG_PREPAUSE = 0x100;

inline constexpr int FLAG_STRESS_END = 0x200;	  // full stress if at end of clause
inline constexpr int FLAG_STRESS_END2 = 0x400;	  // full stress if at end of clause, or only followed by unstressed
inline constexpr int FLAG_UNSTRESS_END = 0x800;	  // reduce stress at end of clause
inline constexpr int FLAG_SPELLWORD = 0x1000;	  // re-translate the word as individual letters, separated by spaces
inline constexpr int FLAG_ACCENT_BEFORE = 0x1000; // say this accent name before the letter name
inline constexpr int FLAG_ABBREV = 0x2000;	 // spell as letters, even with a vowel, OR use specified pronunciation rather than split into letters
inline constexpr int FLAG_DOUBLING = 0x4000; // doubles the following consonant

inline constexpr int BITNUM_FLAG_ALT = 14;		 // bit number of FLAG_ALT_TRANS - 1
inline constexpr int FLAG_ALT_TRANS = 0x8000;	 // language specific
inline constexpr int FLAG_ALT2_TRANS = 0x10000;	 // language specific
inline constexpr int FLAG_ALT3_TRANS = 0x20000;	 // language specific
inline constexpr int FLAG_ALT7_TRANS = 0x200000; // language specific

inline constexpr int FLAG_COMBINE = 0x800000;			   // combine with the next word
inline constexpr int FLAG_ALLOW_DOT = 0x01000000;		   // ignore '.' after word (abbreviation)
inline constexpr int FLAG_NEEDS_DOT = 0x02000000;		   // only if the word is followed by a dot
inline constexpr int FLAG_WAS_UNPRONOUNCABLE = 0x04000000; // the unpronounceable routine was used
inline constexpr int FLAG_MAX3 = 0x08000000;			   // limit to 3 repeats
inline constexpr int FLAG_PAUSE1 = 0x10000000;			   // shorter prepause
inline constexpr int FLAG_TEXTMODE = 0x20000000;		   // word translates to replacement text, not phonemes
inline constexpr int BITNUM_FLAG_TEXTMODE = 29;

inline constexpr int FLAG_FOUND_ATTRIBUTES = 0x40000000; // word was found in the dictionary list (has attributes)
inline constexpr unsigned FLAG_FOUND = 0x80000000;		 // pronunciation was found in the dictionary list

// dictionary flags, word 2
inline constexpr int FLAG_VERBF = 0x1;		 // verb follows
inline constexpr int FLAG_VERBSF = 0x2;		 // verb follows, may have -s suffix
inline constexpr int FLAG_NOUNF = 0x4;		 // noun follows
inline constexpr int FLAG_PASTF = 0x8;		 // past tense follows
inline constexpr int FLAG_VERB = 0x10;		 // pronunciation for verb
inline constexpr int FLAG_NOUN = 0x20;		 // pronunciation for noun
inline constexpr int FLAG_PAST = 0x40;		 // pronunciation for past tense
inline constexpr int FLAG_VERB_EXT = 0x100;	 // extend the 'verb follows'
inline constexpr int FLAG_CAPITAL = 0x200;	 // pronunciation if initial letter is upper case
inline constexpr int FLAG_ALLCAPS = 0x400;	 // only if the word is all capitals
inline constexpr int FLAG_ACCENT = 0x800;	 // character name is base-character name + accent name
inline constexpr int FLAG_SENTENCE = 0x2000; // only if the clause is a sentence
inline constexpr int FLAG_ONLY = 0x4000;
inline constexpr int FLAG_ONLY_S = 0x8000;
inline constexpr int FLAG_STEM = 0x10000;			  // must have a suffix
inline constexpr int FLAG_ATEND = 0x20000;			  // use this pronunciation if at end of clause
inline constexpr int FLAG_ATSTART = 0x40000;		  // use this pronunciation if at start of clause
inline constexpr int FLAG_NATIVE = 0x80000;			  // not if we've switched translators
inline constexpr int FLAG_LOOKUP_SYMBOL = 0x40000000; // to indicate called from Lookup()

inline constexpr int BITNUM_FLAG_ALLCAPS = 0x2a;
inline constexpr int BITNUM_FLAG_HYPHENATED = 0x2c;
inline constexpr int BITNUM_FLAG_ONLY = 0x2e;
inline constexpr int BITNUM_FLAG_ONLY_S = 0x2f;

// wordflags, flags in source word
inline constexpr int FLAG_ALL_UPPER = 0x1;	 // no lower case letters in the word
inline constexpr int FLAG_FIRST_UPPER = 0x2; // first letter is upper case
inline constexpr int FLAG_UPPERS = 0x3;		 // FLAG_ALL_UPPER | FLAG_FIRST_UPPER
inline constexpr int FLAG_HAS_PLURAL = 0x4;	 // upper-case word with s or 's lower-case ending
inline constexpr int FLAG_PHONEMES = 0x8;	 // word is phonemes
inline constexpr int FLAG_LAST_WORD = 0x10;	 // last word in clause
inline constexpr int FLAG_EMBEDDED = 0x40;	 // word is preceded by embedded commands
inline constexpr int FLAG_HYPHEN = 0x80;
inline constexpr int FLAG_NOSPACE = 0x100;	  // word is not separated from previous word by a space
inline constexpr int FLAG_FIRST_WORD = 0x200; // first word in clause
inline constexpr int FLAG_FOCUS = 0x400;	  // the focus word of a clause
inline constexpr int FLAG_EMPHASIZED = 0x800;
inline constexpr int FLAG_EMPHASIZED2 = 0xc00; // FLAG_FOCUS | FLAG_EMPHASIZED
inline constexpr int FLAG_DONT_SWITCH_TRANSLATOR = 0x1000;
inline constexpr int FLAG_SUFFIX_REMOVED = 0x2000;
inline constexpr int FLAG_HYPHEN_AFTER = 0x4000;
inline constexpr int FLAG_ORDINAL = 0x8000;			   // passed to TranslateNumber() to indicate an ordinal number
inline constexpr int FLAG_HAS_DOT = 0x10000;		   // dot after this word
inline constexpr int FLAG_COMMA_AFTER = 0x20000;	   // comma after this word
inline constexpr int FLAG_MULTIPLE_SPACES = 0x40000;   // word is preceded by multiple spaces, newline, or tab
inline constexpr int FLAG_INDIVIDUAL_DIGITS = 0x80000; // speak number as individual digits
inline constexpr int FLAG_DELETE_WORD = 0x100000;	   // don't speak this word, it has been spoken as part of the previous word
inline constexpr int FLAG_CHAR_REPLACED = 0x200000;	   // characters have been replaced by .replace in the *_rules
inline constexpr int FLAG_TRANSLATOR2 = 0x400000;	   // retranslating using a different language
inline constexpr int FLAG_PREFIX_REMOVED = 0x800000;   // a prefix has been removed from this word

inline constexpr int FLAG_SUFFIX_VOWEL = 0x08000000; // remember an initial vowel from the suffix
inline constexpr int FLAG_NO_TRACE = 0x10000000;	 // passed to TranslateRules() to suppress dictionary lookup printout
inline constexpr int FLAG_NO_PREFIX = 0x20000000;
inline constexpr unsigned FLAG_UNPRON_TEST = 0x80000000; // do unpronounability test on the beginning of the word

// prefix/suffix flags (bits 8 to 14, bits 16 to 22) don't use 0x8000, 0x800000
inline constexpr int SUFX_E = 0x0100;  // e may have been added
inline constexpr int SUFX_I = 0x0200;  // y may have been changed to i
inline constexpr int SUFX_P = 0x0400;  // prefix
inline constexpr int SUFX_V = 0x0800;  // suffix means use the verb form pronunciation
inline constexpr int SUFX_D = 0x1000;  // previous letter may have been doubled
inline constexpr int SUFX_F = 0x2000;  // verb follows
inline constexpr int SUFX_Q = 0x4000;  // don't retranslate
inline constexpr int SUFX_T = 0x10000; // don't affect the stress position in the stem
inline constexpr int SUFX_B = 0x20000; // break, this character breaks the word into stem and suffix (used with SUFX_P)
inline constexpr int SUFX_A = 0x40000; // remember that the suffix starts with a vowel
inline constexpr int SUFX_M = 0x80000; // bit 19, allow multiple suffixes

inline constexpr int SUFX_UNPRON = 0x8000; // used to return $unpron flag from *_rules

inline constexpr int FLAG_ALLOW_TEXTMODE = 0x02; // allow dictionary to translate to text rather than phonemes
inline constexpr int FLAG_SUFX = 0x04;
inline constexpr int FLAG_SUFX_S = 0x08;
inline constexpr int FLAG_SUFX_E_ADDED = 0x10;

// codes in dictionary rules
inline constexpr int RULE_PRE = 1;
inline constexpr int RULE_POST = 2;
inline constexpr int RULE_PHONEMES = 3;
inline constexpr int RULE_PH_COMMON = 4; // At start of rule. Its phoneme string is used by subsequent rules
inline constexpr int RULE_CONDITION = 5; // followed by condition number (byte)
inline constexpr int RULE_GROUP_START = 6;
inline constexpr int RULE_GROUP_END = 7;
inline constexpr int RULE_PRE_ATSTART = 8; // as RULE_PRE but also match with 'start of word'
inline constexpr int RULE_LINENUM = 9;	   // next 2 bytes give a line number, for debugging purposes

inline constexpr int RULE_STRESSED = 10;	 // &
inline constexpr int RULE_DOUBLE = 11;		 // %
inline constexpr int RULE_INC_SCORE = 12;	 // +
inline constexpr int RULE_DEL_FWD = 13;		 // #
inline constexpr int RULE_ENDING = 14;		 // S
inline constexpr int RULE_DIGIT = 15;		 // D digit
inline constexpr int RULE_NONALPHA = 16;	 // Z non-alpha
inline constexpr int RULE_LETTERGP = 17;	 // A B C H F G Y   letter group number
inline constexpr int RULE_LETTERGP2 = 18;	 // L + letter group number
inline constexpr int RULE_CAPITAL = 19;		 // !   word starts with a capital letter
inline constexpr int RULE_REPLACEMENTS = 20; // section for character replacements
inline constexpr int RULE_SYLLABLE = 21;	 // @
inline constexpr int RULE_SKIPCHARS = 23;	 // J
inline constexpr int RULE_NO_SUFFIX = 24;	 // N
inline constexpr int RULE_NOTVOWEL = 25;	 // K
inline constexpr int RULE_IFVERB = 26;		 // V
inline constexpr int RULE_DOLLAR = 28;		 // $ commands
inline constexpr int RULE_NOVOWELS = 29;	 // X no vowels up to word boundary
inline constexpr int RULE_SPELLING = 31;	 // W while spelling letter-by-letter
inline constexpr int RULE_LAST_RULE = 31;
// Rule codes above 31 are the ASCII code representation of the character
// used to specify the rule.
inline constexpr int RULE_SPACE = 32;	  // ascii space
inline constexpr int RULE_DEC_SCORE = 60; // <

inline constexpr int DOLLAR_UNPR = 0x01;
inline constexpr int DOLLAR_NOPREFIX = 0x02;
inline constexpr int DOLLAR_LIST = 0x03;

inline constexpr int LETTERGP_A = 0;
inline constexpr int LETTERGP_B = 1;
inline constexpr int LETTERGP_C = 2;
inline constexpr int LETTERGP_H = 3;
inline constexpr int LETTERGP_F = 4;
inline constexpr int LETTERGP_G = 5;
inline constexpr int LETTERGP_Y = 6;
inline constexpr int LETTERGP_VOWEL2 = 7;

// Punctuation types returned by ReadClause()
//@{

inline constexpr int CLAUSE_PAUSE = 0x00000FFF;					 // pause (x 10mS)
inline constexpr int CLAUSE_INTONATION_TYPE = 0x00007000;		 // intonation type
inline constexpr int CLAUSE_OPTIONAL_SPACE_AFTER = 0x00008000;	 // don't need space after the punctuation
inline constexpr int CLAUSE_TYPE = 0x000F0000;					 // phrase type
inline constexpr int CLAUSE_PUNCTUATION_IN_WORD = 0x00100000;	 // punctuation character can be inside a word (Armenian)
inline constexpr int CLAUSE_SPEAK_PUNCTUATION_NAME = 0x00200000; // speak the name of the punctuation character
inline constexpr int CLAUSE_DOT_AFTER_LAST_WORD = 0x00400000;	 // dot after the last word
inline constexpr int CLAUSE_PAUSE_LONG = 0x00800000;			 // x 320mS to the CLAUSE_PAUSE value

inline constexpr int CLAUSE_INTONATION_FULL_STOP = 0x00000000;
inline constexpr int CLAUSE_INTONATION_COMMA = 0x00001000;
inline constexpr int CLAUSE_INTONATION_QUESTION = 0x00002000;
inline constexpr int CLAUSE_INTONATION_EXCLAMATION = 0x00003000;
inline constexpr int CLAUSE_INTONATION_NONE = 0x00004000;

inline constexpr int CLAUSE_TYPE_NONE = 0x00000000;
inline constexpr int CLAUSE_TYPE_EOF = 0x00010000;
inline constexpr int CLAUSE_TYPE_VOICE_CHANGE = 0x00020000;
inline constexpr int CLAUSE_TYPE_CLAUSE = 0x00040000;
inline constexpr int CLAUSE_TYPE_SENTENCE = 0x00080000;

inline constexpr int CLAUSE_NONE = ( 0 | CLAUSE_INTONATION_NONE | CLAUSE_TYPE_NONE );
inline constexpr int CLAUSE_PARAGRAPH = ( 70 | CLAUSE_INTONATION_FULL_STOP | CLAUSE_TYPE_SENTENCE );
inline constexpr int CLAUSE_EOF = ( 40 | CLAUSE_INTONATION_FULL_STOP | CLAUSE_TYPE_SENTENCE | CLAUSE_TYPE_EOF );
inline constexpr int CLAUSE_VOICE = ( 0 | CLAUSE_INTONATION_NONE | CLAUSE_TYPE_VOICE_CHANGE );
inline constexpr int CLAUSE_PERIOD = ( 40 | CLAUSE_INTONATION_FULL_STOP | CLAUSE_TYPE_SENTENCE );
inline constexpr int CLAUSE_COMMA = ( 20 | CLAUSE_INTONATION_COMMA | CLAUSE_TYPE_CLAUSE );
inline constexpr int CLAUSE_SHORTCOMMA = ( 4 | CLAUSE_INTONATION_COMMA | CLAUSE_TYPE_CLAUSE );
inline constexpr int CLAUSE_SHORTFALL = ( 4 | CLAUSE_INTONATION_FULL_STOP | CLAUSE_TYPE_CLAUSE );
inline constexpr int CLAUSE_QUESTION = ( 40 | CLAUSE_INTONATION_QUESTION | CLAUSE_TYPE_SENTENCE );
inline constexpr int CLAUSE_EXCLAMATION = ( 45 | CLAUSE_INTONATION_EXCLAMATION | CLAUSE_TYPE_SENTENCE );
inline constexpr int CLAUSE_COLON = ( 30 | CLAUSE_INTONATION_FULL_STOP | CLAUSE_TYPE_CLAUSE );
inline constexpr int CLAUSE_SEMICOLON = ( 30 | CLAUSE_INTONATION_COMMA | CLAUSE_TYPE_CLAUSE );

//@}


inline constexpr int CHAR_EMPHASIS = 0x0530;	// this is an unused character code
inline constexpr int CHAR_COMMA_BREAK = 0x0557; // unused character code

// Rule:
// [4] [match] [1 pre] [2 post] [3 phonemes] 0
//     match 1 pre 2 post 0     - use common phoneme string
//     match 1 pre 2 post 3 0   - empty phoneme string

// used to mark words with the source[] buffer
struct WORD_TAB {
	unsigned int flags;
	unsigned short start;
	unsigned char pre_pause;
	unsigned short sourceix;
	unsigned char length;
};

struct ALPHABET {
	const char* name;
	int offset;
	unsigned int range_min, range_max;
	int language;
	int flags;
};

// alphabet flags
inline constexpr int AL_DONT_NAME = 0x01;	// don't speak the alphabet name
inline constexpr int AL_NOT_LETTERS = 0x02; // don't use the language for speaking letters
inline constexpr int AL_WORDS = 0x04;		// use the language to speak words
inline constexpr int AL_NOT_CODE = 0x08;	// don't speak the character code
inline constexpr int AL_NO_SYMBOL = 0x10;	// don't repeat "symbol" or "character"

inline constexpr int N_LOPTS = 18;
inline constexpr int LOPT_DIERESES = 0;
// 1=remove [:] from unstressed syllables, 2= remove from unstressed or non-penultimate syllables
// bit 4=0, if stress < 4,  bit 4=1, if not the highest stress in the word
inline constexpr int LOPT_IT_LENGTHEN = 1;

// 1=german
inline constexpr int LOPT_PREFIXES = 2;

// non-zero, change voiced/unoiced to match last consonant in a cluster
// bit 0=use regressive voicing
// bit 1=LANG=cz,bg  don't propagate over [v]
// bit 2=don't propagate acress word boundaries
// bit 3=LANG=pl,  propagate over liquids and nasals
// bit 4=LANG=cz,sk  don't propagate to [v]
// bit 8=devoice word-final consonants
inline constexpr int LOPT_REGRESSIVE_VOICING = 3;

// 0=default, 1=no check, other allow this character as an extra initial letter (default is 's')
inline constexpr int LOPT_UNPRONOUNCABLE = 4;

// increase this to prevent sonorants being shortened before shortened (eg. unstressed) vowels
inline constexpr int LOPT_SONORANT_MIN = 5;

// bit 0: don't break vowels at word boundary
inline constexpr int LOPT_WORD_MERGE = 6;


// max. amplitude for vowel at the end of a clause
inline constexpr int LOPT_MAXAMP_EOC = 7;

// bit 0=reduce even if phonemes are specified in the **_list file
// bit 1=don't reduce the strongest vowel in a word which is marked 'unstressed'
inline constexpr int LOPT_REDUCE = 8;

// LANG=cs,sk  combine some prepositions with the following word, if the combination has N or fewer syllables
// bits 0-3  N syllables
// bit 4=only if the second word has $alt attribute
// bit 5=not if the second word is end-of-sentence
inline constexpr int LOPT_COMBINE_WORDS = 9;

// 1 = stressed syllable is indicated by capitals
inline constexpr int LOPT_CAPS_IN_WORD = 10;

// Call ApplySpecialAttributes() if $alt or $alt2 is set for a word
// bit 1: stressed syllable: $alt change [e],[o] to [E],[O],  $alt2 change [E],[O] to [e],[o]
inline constexpr int LOPT_ALT = 11;

// pause for bracket (default=4), also see LOPT_BRACKET_PAUSE_ANNOUNCED
inline constexpr int LOPT_BRACKET_PAUSE = 12;

// bit 1, don't break clause before annoucning . ? !
inline constexpr int LOPT_ANNOUNCE_PUNCT = 13;

// recognize long vowels (0 = don't recognize)
inline constexpr int LOPT_LONG_VOWEL_THRESHOLD = 14;

// bit 0:  Don't allow suffices if there is no previous syllable
inline constexpr int LOPT_SUFFIX = 15;

// bit 0  Apostrophe at start of word is part of the word
// bit 1  Apostrophe at end of word is part of the word
inline constexpr int LOPT_APOSTROPHE = 16;

// pause when announcing bracket names (default=2), also see LOPT_BRACKET_PAUSE
inline constexpr int LOPT_BRACKET_PAUSE_ANNOUNCED = 17;

// stress_rule
inline constexpr int STRESSPOSN_1L = 0;		  // 1st syllable
inline constexpr int STRESSPOSN_2L = 1;		  // 2nd syllable
inline constexpr int STRESSPOSN_2R = 2;		  // penultimate
inline constexpr int STRESSPOSN_1R = 3;		  // final syllable
inline constexpr int STRESSPOSN_3R = 4;		  // antipenultimate
inline constexpr int STRESSPOSN_SYLCOUNT = 5; // stress depends on syllable count
inline constexpr int STRESSPOSN_1RH = 6;	  // last heaviest syllable, excluding final syllable
inline constexpr int STRESSPOSN_1RU = 7;	  // stress on the last syllable, before any explicitly unstressed syllable
inline constexpr int STRESSPOSN_2LLH = 8;	  // first syllable, unless it is a light syllable followed by a heavy syllable
inline constexpr int STRESSPOSN_ALL = 9;	  // mark all stressed
inline constexpr int STRESSPOSN_GREENLANDIC = 12;
inline constexpr int STRESSPOSN_1SL = 13; // 1st syllable, unless 1st vowel is short and 2nd is long
inline constexpr int STRESSPOSN_EU = 15;  // If more than 2 syllables: primary stress in second syllable and secondary on last.

// stress rule options (langopts.stress_flags), number syntax options (numbers, numbers2) and where numbers break into groups (break_numbers)
inline constexpr int S_NO_DIM = 0x02;
inline constexpr int S_FINAL_DIM = 0x04;
inline constexpr int S_FINAL_DIM_ONLY = 0x06;
inline constexpr int S_FINAL_NO_2 = 0x10;
inline constexpr int S_NO_AUTO_2 = 0x20;
inline constexpr int S_2_TO_HEAVY = 0x40;
inline constexpr int S_FIRST_PRIMARY = 0x80;
inline constexpr int S_FINAL_VOWEL_UNSTRESSED = 0x100;
inline constexpr int S_FINAL_SPANISH = 0x200;
inline constexpr int S_2_SYL_2 = 0x1000;
inline constexpr int S_INITIAL_2 = 0x2000;
inline constexpr int S_MID_DIM = 0x10000;
inline constexpr int S_PRIORITY_STRESS = 0x20000;
inline constexpr int S_EO_CLAUSE1 = 0x40000;
inline constexpr int S_FINAL_LONG = 0x80000;
inline constexpr int S_HYPEN_UNSTRESS = 0x100000;
inline constexpr int S_NO_EOC_LENGTHEN = 0x200000;
inline constexpr int NUM_DEFAULT = 0x00000001;			 // enable number processing; use if no other NUM_ option is specified
inline constexpr int NUM_THOUS_SPACE = 0x00000004;		 // thousands separator must be space
inline constexpr int NUM_DECIMAL_COMMA = 0x00000008;	 // , decimal separator, not .
inline constexpr int NUM_SWAP_TENS = 0x00000010;		 // use three-and-twenty rather than twenty-three
inline constexpr int NUM_AND_UNITS = 0x00000020;		 // 'and' between tens and units
inline constexpr int NUM_HUNDRED_AND = 0x00000040;		 // add "and" after hundred or thousand
inline constexpr int NUM_SINGLE_AND = 0x00000080;		 // don't have "and" both after hundreds and also between tens and units
inline constexpr int NUM_SINGLE_STRESS = 0x00000100;	 // only one primary stress in tens+units
inline constexpr int NUM_SINGLE_VOWEL = 0x00000200;		 // only one vowel between tens and units
inline constexpr int NUM_OMIT_1_HUNDRED = 0x00000400;	 // omit "one" before "hundred"
inline constexpr int NUM_1900 = 0x00000800;				 // say 19** as nineteen hundred
inline constexpr int NUM_ALLOW_SPACE = 0x00001000;		 // allow space as thousands separator (in addition to langopts.thousands_sep)
inline constexpr int NUM_DFRACTION_BITS = 0x0000e000;	 // post-decimal-digits 0=single digits, 1=(LANG=it) 2=(LANG=pl) 3=(LANG=ro)
inline constexpr int NUM_ORDINAL_DOT = 0x00010000;		 // dot after number indicates ordinal
inline constexpr int NUM_NOPAUSE = 0x00020000;			 // don't add pause after a number
inline constexpr int NUM_AND_HUNDRED = 0x00040000;		 // 'and' before hundreds
inline constexpr int NUM_THOUSAND_AND = 0x00080000;		 // 'and' after thousands if there are no hundreds
inline constexpr int NUM_VIGESIMAL = 0x00100000;		 // vigesimal number, if tens are not found
inline constexpr int NUM_OMIT_1_THOUSAND = 0x00200000;	 // omit "one" before "thousand"
inline constexpr int NUM_ZERO_HUNDRED = 0x00400000;		 // say "zero" before hundred
inline constexpr int NUM_HUNDRED_AND_DIGIT = 0x00800000; // add "and" after hundreds and thousands, only if there are digits and no tens
inline constexpr int NUM_ROMAN = 0x01000000;			 // recognize roman numbers
inline constexpr int NUM_ROMAN_CAPITALS = 0x02000000;	 // Roman numbers only if upper case
inline constexpr int NUM_ROMAN_AFTER = 0x04000000;		 // say "roman" after the number, not before
inline constexpr int NUM_ROMAN_ORDINAL = 0x08000000;	 // Roman numbers are ordinal numbers
inline constexpr int NUM_SINGLE_STRESS_L = 0x10000000;	 // only one primary stress in tens+units (on the tens)
inline constexpr int NUM_DFRACTION_1 = 0x00002000;
inline constexpr int NUM_DFRACTION_2 = 0x00004000;
inline constexpr int NUM_DFRACTION_3 = 0x00006000;
inline constexpr int NUM_DFRACTION_4 = 0x00008000;
inline constexpr int NUM_DFRACTION_5 = 0x0000a000;
inline constexpr int NUM_DFRACTION_6 = 0x0000c000;
inline constexpr int NUM_DFRACTION_7 = 0x0000e000;			  // lang=si, alternative form of number for decimal fraction digits (except the last)
inline constexpr int NUM2_THOUSANDS_VAR_BITS = 0x000001c0;	  // use different forms of thousand, million, etc (M MA MB)
inline constexpr int NUM2_SWAP_THOUSANDS = 0x00000200;		  // say "thousand" and "million" before its number, not after
inline constexpr int NUM2_ORDINAL_NO_AND = 0x00000800;		  // don't say 'and' between tens and units for ordinal numbers
inline constexpr int NUM2_MULTIPLE_ORDINAL = 0x00001000;	  // use ordinal form of hundreds and tens as well as units
inline constexpr int NUM2_NO_TEEN_ORDINALS = 0x00002000;	  // don't use 11-19 numbers to make ordinals
inline constexpr int NUM2_MYRIADS = 0x00004000;				  // use myriads (groups of 4 digits) not thousands (groups of 3)
inline constexpr int NUM2_ENGLISH_NUMERALS = 0x00008000;	  // speak (non-replaced) English numerals in English
inline constexpr int NUM2_PERCENT_BEFORE = 0x00010000;		  // say "%" before the number
inline constexpr int NUM2_OMIT_1_HUNDRED_ONLY = 0x00020000;	  // omit "one" before hundred only if there are no previous digits
inline constexpr int NUM2_ORDINAL_AND_THOUSANDS = 0x00040000; // same variant for ordinals and thousands (#o = #a)
inline constexpr int NUM2_ORDINAL_DROP_VOWEL =
	0x00080000;									  // drop final vowel from cardial number before adding ordinal suffix (currently only tens and units)
inline constexpr int NUM2_ZERO_TENS = 0x00100000; // say zero tens
inline constexpr int NUM2_THOUSANDPLEX_VAR_THOUSANDS = 0x00000002;
inline constexpr int NUM2_THOUSANDPLEX_VAR_MILLIARDS = 0x00000008;
inline constexpr int NUM2_THOUSANDPLEX_VAR_ALL = 0x0000001e;
inline constexpr int NUM2_THOUSANDS_VAR1 = 0x00000040;
inline constexpr int NUM2_THOUSANDS_VAR2 = 0x00000080;
inline constexpr int NUM2_THOUSANDS_VAR3 = 0x000000c0;
inline constexpr int NUM2_THOUSANDS_VAR4 = 0x00000100; // plural forms for millions, etc.
inline constexpr int NUM2_THOUSANDS_VAR5 = 0x00000140;
inline constexpr int BREAK_THOUSANDS = 0x49249248;	// b  b  b  b  b  b  b  b  b  b  b  // 10,000,000,000,000,000,000,000,000,000,000
inline constexpr int BREAK_MYRIADS = 0x11111110;	// b   b   b   b   b   b   b   b    // 1000,0000,0000,0000,0000,0000,0000,0000
inline constexpr unsigned BREAK_LAKH = 0xaaaaaaa8;	// b  b b b b b b b b b b b b b b b // 10,00,00,00,00,00,00,00,00,00,00,00,00,00,00,000
inline constexpr int BREAK_LAKH_BN = 0x24924aa8;	// b  b b b b b  b  b  b  b  b  b   // 100,000,000,000,000,000,000,00,00,00,00,000
inline constexpr int BREAK_LAKH_DV = 0x000014a8;	// b  b b b  b b                    // 100,00,000,00,00,000
inline constexpr int BREAK_LAKH_HI = 0x00014aa8;	// b  b b b b b  b b                // 100,00,000,00,00,00,00,000
inline constexpr int BREAK_LAKH_UR = 0x000052a8;	// b  b b b b  b b                  // 100,00,000,00,00,00,000
inline constexpr int BREAK_INDIVIDUAL = 0x00000018; // b  bb                            // 100,0,000

struct LANGUAGE_OPTIONS {
	// bits0-2  separate words with (1=pause_vshort, 2=pause_short, 3=pause, 4=pause_long 5=[?] phonemme)
	// bit 3=don't use linking phoneme
	// bit4=longer pause before STOP, VSTOP,FRIC
	// bit5=length of a final vowel doesn't depend on the next phoneme
	int word_gap;
	int vowel_pause;
	int stress_rule; // see #defines for STRESSPOSN_*

	// bit1=don't set diminished stress,
	// bit2=mark unstressed final syllables as diminished

	// bit3=set consecutive unstressed syllables in unstressed words to diminished, but not in stressed words

	// bit4=don't allow secondary stress on last syllable

	// bit5-don't use automatic secondary stress

	// bit6=light syllable followed by heavy, move secondary stress to the heavy syllable. LANG=Finnish

	// bit7=if more than one primary stress, make the subsequent primaries to secondary stress

	// bit8=don't apply default stress to a word-final vowel

	// bit9=stress last syllable if it doesn't end in vowel or "s" or "n"  LANG=Spanish

	// bit12= In a 2-syllable word, if one has primary stress then give the other secondary stress

	// bit13= If there is only one syllable before the primary stress, give it a secondary stress

	// bit 16= Set (not first or last) syllables to diminished stress

	// bit17= "priority" stress reduces other primary stress to "unstressed" not "secondary"

	// bit18= don't lengthen short vowels more than long vowels at end-of-clause

	// bit19=stress on final syllable if it has a long vowel, but previous syllable has a short vowel


	// bit20= hyphenated words, 2nd part is unstressed

	// bit21= don't lengthen vowels at end-of-clause

	// bit15= Give stress to the first unstressed syllable

	int stress_flags;
	int unstressed_wd1; // stress for $u word of 1 syllable
	int unstressed_wd2; // stress for $u word of >1 syllable
	int param[N_LOPTS];


	int numbers;


	int numbers2;

	// Bit 2^n is set if 10^n separates a number grouping (max n=31).
	//                                      0         1         2         3
	//                                  n = 01234567890123456789012345678901

	unsigned break_numbers; // which digits to break the number into thousands, millions, etc (Hindi has 100,000 not 1,000,000)
	int max_roman;
	int min_roman;
	int thousands_sep;
	int decimal_sep;
	int max_digits;					   // max number of digits which can be spoken as an integer number (rather than individual digits)
	const char* ordinal_indicator;	   // UTF-8 string
	const unsigned char* roman_suffix; // add this (ordinal) suffix to Roman numbers (LANG=an)

	// bit 0, accent name before the letter name, bit 1 "capital" after letter name
	int accents;

	int tone_language; // 1=tone language
	char max_initial_consonants;
	bool spelling_stress;
	char tone_numbers;
	char ideographs;					// treat as separate words
	bool textmode;						// the meaning of FLAG_TEXTMODE is reversed (to save data when *_list file is compiled)
	char dotless_i;						// uses letter U+0131
	int listx;							// compile *_listx after *list
	const unsigned char* replace_chars; // characters to be substitutes
	int our_alphabet;					// offset for main alphabet (if not set in letter_bits_offset)
	int alt_alphabet;					// offset for another language to recognize
	int alt_alphabet_lang;				// language for the alt_alphabet
	int suffix_add_e;					// replace a suffix (which has the SUFX_E flag) with this character
	bool lowercase_sentence;			// when true, a period . causes a sentence stop even if next character is lowercase
};

struct Translator {
	LANGUAGE_OPTIONS langopts;
	int translator_name;
	int transpose_max;
	int transpose_min;
	const char* transpose_map;
	char dictionary_name[40];

	char phonemes_repeat[20];
	int phonemes_repeat_count;
	int phoneme_tab_ix;

	int dict_condition; // conditional apply some pronunciation rules and dict.lookups
	int dict_min_size;
	const wchar_t* char_plus_apostrophe; // single chars + apostrophe treated as words
	const wchar_t* punct_within_word;	 // allow these punctuation characters within words
	const unsigned short* chars_ignore;

	// holds properties of characters: vowel, consonant, etc for pronunciation rules
	unsigned char letter_bits[256];
	int letter_bits_offset;
	const wchar_t* letter_groups[8];

	char* data_dictrules;			 // language_1   translation rules file
	char* data_dictlist;			 // language_2   dictionary lookup file
	char* dict_hashtab[N_HASH_DICT]; // hash table to index dictionary lookup file
	char* letterGroups[N_LETTER_GROUPS];

	// groups1 and groups2 are indexes into data_dictrules, set up by InitGroups()
	// the two-letter rules for each letter must be consecutive in the language_rules source

	char* groups1[256];						  // translation rule lists, index by single letter
	char* groups3[128];						  // index by offset letter
	char* groups2[N_RULE_GROUP2];			  // translation rule lists, indexed by two-letter pairs
	unsigned int groups2_name[N_RULE_GROUP2]; // the two letter pairs for groups2[]
	int n_groups2;							  // number of groups2[] entries used

	unsigned char groups2_count[256]; // number of 2 letter groups for this initial letter
	unsigned char groups2_start[256]; // index into groups2
	const short* frequent_pairs;	  // list of frequent pairs of letters, for use in compressed *_list

	int expect_verb;
	int expect_past; // expect past tense
	int expect_verb_s;
	int expect_noun;
	int prev_last_stress;
	char* clause_end;

	int word_vowel_count;	 // number of vowels so far
	int word_stressed_count; // number of vowels so far which could be stressed

	int clause_upper_count; // number of upper case letters in the clause
	int clause_lower_count; // number of lower case letters in the clause

	int prepause_timeout;
	int end_stressed_vowel; // word ends with stressed vowel
	int prev_dict_flags[2]; // dictionary flags from previous word
	int clause_terminator;
};


inline constexpr int N_PHONEME_LIST = 1000; // enough for source[N_TR_SOURCE] full of text, else it will truncate

inline constexpr int SFLAG_SEQCONTINUE = 0x01;	  // a liquid or nasal after a vowel, but not followed by a vowel
inline constexpr int SFLAG_EMBEDDED = 0x02;		  // there are embedded commands before this phoneme
inline constexpr int SFLAG_SYLLABLE = 0x04;		  // vowel or syllabic consonant
inline constexpr int SFLAG_LENGTHEN = 0x08;		  // lengthen symbol : included after this phoneme
inline constexpr int SFLAG_DICTIONARY = 0x10;	  // the pronunciation of this word was listed in the xx_list dictionary
inline constexpr int SFLAG_SWITCHED_LANG = 0x20;  // this word uses phonemes from a different language
inline constexpr int SFLAG_PROMOTE_STRESS = 0x40; // this unstressed word can be promoted to stressed

inline constexpr int SFLAG_NEXT_PAUSE = 0x2000; // consider next phoneme as pause

// embedded command numbers


// a clause translated into phoneme codes (first stage)
struct PHONEME_LIST2 {
	unsigned short synthflags;
	unsigned char phcode;
	unsigned char stresslevel;
	unsigned short sourceix;  // ix into the original source text string, only set at the start of a word
	unsigned char wordstress; // the highest level stress in this word
	unsigned char tone_ph;	  // tone phoneme to use with this vowel
};

inline constexpr int PHLIST_START_OF_WORD = 1;
inline constexpr int PHLIST_END_OF_CLAUSE = 2;
inline constexpr int PHLIST_START_OF_SENTENCE = 4;
inline constexpr int PHLIST_START_OF_CLAUSE = 8;

// the final phoneme list of a clause: the first section is a copy of PHONEME_LIST2
struct PHONEME_LIST {
	unsigned short synthflags;
	unsigned char phcode;
	unsigned char stresslevel;
	unsigned short sourceix;
	unsigned char wordstress;
	unsigned char tone_ph;

	PHONEME_TAB* ph;
	unsigned char type;
	unsigned char newword; // bit flags, see PHLIST_(START|END)_OF_*
};

inline constexpr int pd_ADDWAV = 4;

inline constexpr int N_PHONEME_DATA_PARAM = 16;

// what a phoneme's program yields
struct PHONEME_DATA {
	int pd_param[N_PHONEME_DATA_PARAM]; // set from group 0 instructions
	char ipa_string[18];
};

struct WORD_PH_DATA {
	PHONEME_LIST prev_vowel;
};

// instructions

inline constexpr int INSTN_RETURN = 0x0001;
inline constexpr int INSTN_CONTINUE = 0x0002;

// Group 0 instructions with 8 bit operand.  These values go into bits 8-15 of the instruction
inline constexpr int i_CHANGE_PHONEME = 0x01;
inline constexpr int i_REPLACE_NEXT_PHONEME = 0x02;
inline constexpr int i_INSERT_PHONEME = 0x03;
inline constexpr int i_APPEND_PHONEME = 0x04;
inline constexpr int i_APPEND_IFNEXTVOWEL = 0x05;
inline constexpr int i_VOICING_SWITCH = 0x06;
inline constexpr int i_PAUSE_BEFORE = 0x07;
inline constexpr int i_PAUSE_AFTER = 0x08;
inline constexpr int i_LENGTH_MOD = 0x09;
inline constexpr int i_SET_LENGTH = 0x0a;
inline constexpr int i_LONG_LENGTH = 0x0b;
inline constexpr int i_ADD_LENGTH = 0x0c;
inline constexpr int i_IPA_NAME = 0x0d;

inline constexpr int i_CHANGE_IF = 0x10; // 0x10 to 0x14

// conditions and jumps
inline constexpr int i_CONDITION = 0x2000;
inline constexpr int i_OR = 0x1000; // added to i_CONDITION
inline constexpr int i_NOT = 0x0003;

inline constexpr int i_JUMP = 0x6000;
inline constexpr int i_JUMP_FALSE = 0x6800;
inline constexpr int i_SWITCH_NEXTVOWEL = 0x6a00;
inline constexpr int i_SWITCH_PREVVOWEL = 0x6c00;
inline constexpr int MAX_JUMP = 255; // max jump distance

// multi-word instructions
inline constexpr int i_CALLPH = 0x9100;
inline constexpr int i_PITCHENV = 0x9200;
inline constexpr int i_AMPENV = 0x9300;
inline constexpr int i_VOWELIN = 0xa100;
inline constexpr int i_VOWELOUT = 0xa200;
inline constexpr int i_FMT = 0xb000;
inline constexpr int i_WAV = 0xc000;
inline constexpr int i_VWLSTART = 0xd000;
inline constexpr int i_VWLENDING = 0xe000;
inline constexpr int i_WAVADD = 0xf000;

// conditions
inline constexpr int CONDITION_IS_PHONEME_TYPE = 0x00;
inline constexpr int CONDITION_IS_PLACE_OF_ARTICULATION = 0x20;
inline constexpr int CONDITION_IS_PHFLAG_SET = 0x40;
inline constexpr int CONDITION_IS_OTHER = 0x80;

// other conditions (stress)
inline constexpr int STRESS_IS_DIMINISHED = 0;	 // diminished, unstressed within a word
inline constexpr int STRESS_IS_UNSTRESSED = 1;	 // unstressed, weak
inline constexpr int STRESS_IS_NOT_STRESSED = 2; // default, not stressed
inline constexpr int STRESS_IS_SECONDARY = 3;	 // secondary stress
inline constexpr int STRESS_IS_PRIMARY = 4;		 // primary (main) stress
inline constexpr int STRESS_IS_PRIORITY = 5;	 // replaces primary markers

// other conditions
inline constexpr int isAfterStress = 9;
inline constexpr int isNotVowel = 10;
inline constexpr int isFinalVowel = 11;
inline constexpr int isVoiced = 12; // voiced consonant, or vowel
inline constexpr int isFirstVowel = 13;
inline constexpr int isSecondVowel = 14;
inline constexpr int isTranslationGiven = 16; // phoneme translation given in **_list or as [[...]]
inline constexpr int isBreak = 17;			  // pause phoneme or (stop/vstop/vfric not followed by vowel or (liquid in same word))
inline constexpr int isWordStart = 18;
inline constexpr int isWordEnd = 19;

// the parameters of a phoneme program that the translator reads (PHONEME_DATA.pd_param)
inline constexpr int pd_INSERTPHONEME = i_INSERT_PHONEME;
inline constexpr int pd_APPENDPHONEME = i_APPEND_PHONEME;
inline constexpr int pd_CHANGEPHONEME = i_CHANGE_PHONEME;
inline constexpr int pd_CHANGE_NEXTPHONEME = i_REPLACE_NEXT_PHONEME;


// a rule match in TranslateRules (dictionary.cpp)
struct MatchRecord {
	int points;
	const char* phonemes;
	int end_type;
	char* del_fwd;
};


// espeak_TextToPhonemes's phoneme modes (speak_lib.h)
inline constexpr int espeakPHONEMES_IPA = 0x02;
inline constexpr int espeakPHONEMES_TIE = 0x80;
