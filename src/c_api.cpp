// The C ABI over the C++ API: include/propan2ol/propan2ol_c.h.
#include <propan2ol/propan2ol.h>
#include <propan2ol/propan2ol_c.h>
#include <cstdlib>
#include <cstring>
#include <exception>

struct p2o_phonemizer {
	std::unique_ptr<propan2ol::Phonemizer> impl;
};

namespace {

thread_local std::string last_error;

char* duplicate( const std::string& s ) {
	char* out = static_cast<char*>( std::malloc( s.size() + 1 ) );
	if ( out ) {
		std::memcpy( out, s.c_str(), s.size() + 1 );
	}
	return out;
}

std::string text_or( const char* s, const char* otherwise ) { return s ? s : otherwise; }

} // namespace

extern "C" {

p2o_options p2o_options_default( void ) {
	p2o_options o = {};
	o.struct_size = sizeof( p2o_options );
	return o;
}

p2o_phonemizer* p2o_create( const char* profile, const char* language ) {
	try {
		if ( !language ) {
			last_error = "no language";
			return nullptr;
		}
		std::string error;
		auto impl = propan2ol::Phonemizer::create( text_or( profile, propan2ol::DEFAULT_PROFILE ), language, &error );
		if ( !impl ) {
			last_error = error;
			return nullptr;
		}
		return new p2o_phonemizer{ std::move( impl ) };
	} catch ( const std::exception& e ) {
		last_error = e.what();
		return nullptr;
	}
}

void p2o_destroy( p2o_phonemizer* phonemizer ) { delete phonemizer; }
}

namespace {

// the C options as the C++ ones; false (with last_error set) if they are invalid
bool convert_options( const p2o_options* options, propan2ol::Options& o ) {
	if ( !options ) {
		return true;
	}
	// fields beyond options->struct_size are not there
	const auto has = [&]( size_t end ) { return options->struct_size >= end; };
	o.preserve_punctuation = options->preserve_punctuation != 0;
	o.with_stress = options->with_stress != 0;
	o.strip = options->strip != 0;
	if ( options->language_switch < P2O_KEEP_FLAGS || options->language_switch > P2O_REMOVE_UTTERANCE ) {
		last_error = "bad language_switch";
		return false;
	}
	o.language_switch = static_cast<propan2ol::LanguageSwitch>( options->language_switch );
	if ( has( offsetof( p2o_options, tie ) + sizeof( options->tie ) ) ) {
		o.punctuation_marks = text_or( options->punctuation_marks, "" );
		o.tie = text_or( options->tie, "" );
	}
	if ( has( offsetof( p2o_options, phone_separator ) + sizeof( options->phone_separator ) ) ) {
		o.separator.word = text_or( options->word_separator, " " );
		o.separator.syllable = text_or( options->syllable_separator, "" );
		o.separator.phone = text_or( options->phone_separator, "" );
	}
	return true;
}

} // namespace

extern "C" {

char* p2o_phonemize( const p2o_phonemizer* phonemizer, const char* text, const p2o_options* options ) {
	try {
		if ( !phonemizer || !text ) {
			last_error = "null argument";
			return nullptr;
		}
		propan2ol::Options o;
		if ( !convert_options( options, o ) ) {
			return nullptr;
		}
		return duplicate( phonemizer->impl->phonemize( std::string( text ), o ) );
	} catch ( const std::exception& e ) {
		last_error = e.what();
		return nullptr;
	}
}

char** p2o_phonemize_lines( const p2o_phonemizer* phonemizer, const char* const* lines, size_t n_lines, const p2o_options* options, size_t* n_out ) {
	try {
		if ( !phonemizer || ( !lines && n_lines ) || !n_out ) {
			last_error = "null argument";
			return nullptr;
		}
		propan2ol::Options o;
		if ( !convert_options( options, o ) ) {
			return nullptr;
		}
		std::vector<std::string> in;
		for ( size_t i = 0; i < n_lines; ++i ) {
			in.emplace_back( lines[i] ? lines[i] : "" );
		}
		const std::vector<std::string> out = phonemizer->impl->phonemize( in, o );
		char** result = static_cast<char**>( std::calloc( out.size() + 1, sizeof( char* ) ) );
		if ( !result ) {
			last_error = "out of memory";
			return nullptr;
		}
		for ( size_t i = 0; i < out.size(); ++i ) {
			result[i] = duplicate( out[i] );
		}
		*n_out = out.size();
		return result;
	} catch ( const std::exception& e ) {
		last_error = e.what();
		return nullptr;
	}
}

void p2o_free_lines( char** lines, size_t n ) {
	if ( !lines ) {
		return;
	}
	for ( size_t i = 0; i < n; ++i ) {
		std::free( lines[i] );
	}
	std::free( lines );
}

void p2o_free( void* ptr ) { std::free( ptr ); }

char* p2o_languages( const char* profile ) {
	std::string joined;
	for ( const std::string& l : propan2ol::Phonemizer::languages( text_or( profile, propan2ol::DEFAULT_PROFILE ) ) ) {
		joined += ( joined.empty() ? "" : "\n" ) + l;
	}
	return duplicate( joined );
}

const char* p2o_last_error( void ) { return last_error.c_str(); }
}
