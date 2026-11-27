// Engine creation and the entry points: espeak-ng's espeak_ng_Initialize (the part that loads the phoneme data), the voice selection of
// voices.c reduced to what reaches the phonemes (see LanguageSpec), and espeak_TextToPhonemes.
#include "engine.h"
#include <strings.h>

namespace propan2ol {

namespace {

// A language as espeak's voice file for it configures the translator. Almost all of a voice file is synthesis (pitch, formants, speed, tunes,
// stress lengths and amplitudes, mbrola, variants) and never reaches the phonemes; what does is the translator, its phoneme table and
// dictionary, the dictrules / dict_min / words options and the `replace` rules. Each spec is its voice file reduced to that.
struct Replace {
	int flags;
	const char* old_ph;
	const char* new_ph;
};

struct LanguageSpec {
	const char* code;		 // the code phonemizer's set_voice maps from: the voice's first `language` line
	const char* translator;	 // SelectTranslator's name: the code up to its first '-'
	const char* phonemes;	 // `phonemes`, else the translator name
	const char* dictionary;	 // `dictionary`, else the translator name
	unsigned dict_condition; // `dictrules` as flag bits
	int dict_min_size;		 // `dict_min`
	int word_gap;			 // `words`, or -1 for the translator's default
	Replace replace[3];		 // `replace`
};

const LanguageSpec LANGUAGES[] = {
	{ "en-us", "en", "en-us", "en", ( 1u << 3 ) | ( 1u << 6 ), 0, -1, { { 3, "I", "i" }, { 3, "I2", "i" }, {} } },
	{ "fr-fr", "fr", "fr", "fr", 1u << 1, 0, -1, {} },
	{ "de", "de", "de", "de", 0, 0, -1, {} },
	{ "ja", "ja", "ja", "ja", 0, 0, -1, {} },
	{ "ko", "ko", "ko", "ko", 0, 0, -1, {} },
	{ "cmn-latn-pinyin", "cmn", "cmn", "cmn", 0, 100000, 1, {} },
};

} // namespace

Engine::~Engine() {
	for ( Translator* tr : { translator, translator2, translator3 } ) {
		if ( tr != nullptr ) {
			DeleteTranslator( tr );
		}
	}
}

std::unique_ptr<Engine> Engine::create() {
	auto engine = std::make_unique<Engine>();
	if ( !engine->LoadPhData() ) {
		return nullptr;
	}
	return engine;
}

const char* const* Engine::languages() {
	static const char* codes[sizeof( LANGUAGES ) / sizeof( LANGUAGES[0] ) + 1];
	if ( codes[0] == nullptr ) {
		size_t n = 0;
		for ( const LanguageSpec& s : LANGUAGES ) {
			codes[n++] = s.code;
		}
	}
	return codes;
}

bool Engine::set_language( const char* code ) {
	const LanguageSpec* spec = nullptr;
	for ( const LanguageSpec& s : LANGUAGES ) {
		if ( strcasecmp( code, s.code ) == 0 ) {
			spec = &s;
		}
	}
	if ( spec == nullptr ) {
		return false;
	}

	if ( translator != nullptr ) {
		DeleteTranslator( translator );
		translator = nullptr;
	}

	n_replace_phonemes = 0;
	translator = SelectTranslator( spec->translator );
	const int table = SelectPhonemeTableName( spec->phonemes );
	if ( table < 0 ) {
		return false;
	}
	for ( const Replace& r : spec->replace ) {
		if ( r.old_ph == nullptr ) {
			break;
		}
		const int phon = LookupPhonemeString( r.old_ph );
		if ( phon != 0 && n_replace_phonemes < N_REPLACE_PHONEMES ) {
			replace_phonemes[n_replace_phonemes].old_ph = phon;
			replace_phonemes[n_replace_phonemes].new_ph = LookupPhonemeString( r.new_ph );
			replace_phonemes[n_replace_phonemes++].type = r.flags;
		}
	}
	translator->dict_condition = spec->dict_condition;
	if ( spec->dict_min_size ) {
		translator->dict_min_size = spec->dict_min_size;
	}
	if ( spec->word_gap >= 0 ) {
		translator->langopts.word_gap = spec->word_gap;
	}
	voice_phoneme_tab_ix = table;
	translator->phoneme_tab_ix = table;

	LoadDictionary( translator, spec->dictionary, 0 );
	if ( dictionary_name[0] == 0 ) {
		DeleteTranslator( translator );
		translator = nullptr;
		return false;
	}
	return true;
}

const char* Engine::text_to_phonemes( const void** textptr, int phonememode ) {
	decoder.reset( static_cast<const char*>( *textptr ) );
	TranslateClause( translator, nullptr, nullptr );
	*textptr = decoder.position();
	return GetTranslatedPhonemeString( phonememode );
}

} // namespace propan2ol
