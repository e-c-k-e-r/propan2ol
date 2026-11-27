// the public C++ API: errors, languages, the options, and the C ABI over it
#include <propan2ol/propan2ol.h>
#include <propan2ol/propan2ol_c.h>
#include <cstdio>
#include <cstring>

static int failures = 0;
#define CHECK( cond )                                                                                                                                \
	do {                                                                                                                                             \
		if ( !( cond ) ) {                                                                                                                           \
			std::printf( "FAIL line %d: %s\n", __LINE__, #cond );                                                                                    \
			++failures;                                                                                                                              \
		}                                                                                                                                            \
	} while ( 0 )

int main() {
	using namespace propan2ol;
	std::string error;
	CHECK( !Phonemizer::create( "nope", "en-us", &error ) && error.find( "unknown profile" ) != std::string::npos );
	CHECK( !Phonemizer::create( DEFAULT_PROFILE, "xx", &error ) && error.find( "not available" ) != std::string::npos );
	CHECK( Phonemizer::languages().size() == 6 );

	auto en = Phonemizer::create( DEFAULT_PROFILE, "en-us" );
	CHECK( en );
	Options o;
	o.strip = true;
	o.with_stress = true;
	CHECK( en->phonemize( "hello world", o ) == "həlˈoʊ wˈɜːld" );
	o.separator.phone = "|";
	CHECK( en->phonemize( "hello", o ) == "h|ə|l|ˈoʊ" );
	o = {};
	o.preserve_punctuation = true;
	o.strip = true;
	CHECK( en->phonemize( "yes, no.", o ) == "jɛs, noʊ." );
	CHECK( en->phonemize( std::vector<std::string>{ "a", "b" }, o ).size() == 2 );

	auto fr = Phonemizer::create( DEFAULT_PROFILE, "fr-fr" );
	o = {};
	o.strip = true;
	CHECK( fr->phonemize( "le football", o ).find( "(en)" ) != std::string::npos );
	o.language_switch = LanguageSwitch::RemoveFlags;
	CHECK( fr->phonemize( "le football", o ).find( "(en)" ) == std::string::npos );
	o.language_switch = LanguageSwitch::RemoveUtterance;
	CHECK( fr->phonemize( "le football", o ).empty() );

	// the C ABI
	p2o_phonemizer* c = p2o_create( nullptr, "de" );
	CHECK( c );
	p2o_options co = p2o_options_default();
	co.strip = 1;
	char* out = p2o_phonemize( c, "Guten Tag", &co );
	CHECK( out && std::strcmp( out, "ɡuːtən tɑːk" ) == 0 );
	p2o_free( out );
	const char* lines[] = { "eins", "zwei" };
	size_t n = 0;
	char** many = p2o_phonemize_lines( c, lines, 2, &co, &n );
	CHECK( many && n == 2 );
	p2o_free_lines( many, n );
	p2o_destroy( c );
	CHECK( !p2o_create( nullptr, "xx" ) && std::strstr( p2o_last_error(), "not available" ) );

	std::printf( failures ? "FAIL\n" : "ok\n" );
	return failures != 0;
}
