// dev probe, batch mode: reads "voice<TAB>text" lines on stdin and prints the clauses' IPA (the call vall-e's front-end makes:
// IPA, '_' between phonemes, clauses joined with spaces), one line each. Built twice: against the port, and against the system
// libespeak-ng (-DREFERENCE) so tests/parity/compare.py can diff the two.
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#ifdef REFERENCE
#include <espeak-ng/speak_lib.h>
#else
#include "engine/engine.h"
#endif

int main() {
#ifdef REFERENCE
	if ( espeak_Initialize( AUDIO_OUTPUT_SYNCHRONOUS, 0, nullptr, 0 ) < 0 ) {
		return 1;
	}
#else
	auto engine = propan2ol::Engine::create();
	if ( !engine ) {
		return 1;
	}
#endif
#ifdef REFERENCE
	// phonemizer's set_voice: a language code maps to the identifier of the first voice listing it (after its priority byte)
	std::map<std::string, std::string> by_language;
	for ( const espeak_VOICE** v = espeak_ListVoices( nullptr ); *v; ++v ) {
		const char* l = ( *v )->languages;
		if ( *l ) {
			by_language.emplace( l + 1, ( *v )->identifier );
		}
	}
#endif
	std::string line, voice;
	while ( std::getline( std::cin, line ) ) {
		const auto tab = line.find( '\t' );
		if ( tab == std::string::npos ) {
			std::puts( "" );
			continue;
		}
		const std::string code = line.substr( 0, tab ), text = line.substr( tab + 1 );
#ifdef REFERENCE
		const auto it = by_language.find( code );
		if ( it == by_language.end() ) {
			std::printf( "NO VOICE %s\n", code.c_str() );
			continue;
		}
		const std::string& v = it->second;
#else
		const std::string& v = code;
#endif
		if ( v != voice ) {
			voice = v;
#ifdef REFERENCE
			espeak_SetVoiceByName( v.c_str() );
#else
			engine->set_language( v.c_str() );
#endif
		}
		const void* p = text.c_str();
		std::string out;
		while ( p ) {
#ifdef REFERENCE
			const char* s = espeak_TextToPhonemes( &p, espeakCHARS_UTF8, ( '_' << 8 ) | 0x02 );
#else
			const char* s = engine->text_to_phonemes( &p, ( '_' << 8 ) | 0x02 );
#endif
			if ( s ) {
				out += s;
				out += ' ';
			}
		}
		std::puts( out.c_str() );
	}
}
