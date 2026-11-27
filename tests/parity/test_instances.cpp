// two instances in one process share nothing: interleaved use of an en-us and a fr-fr instance, and instances on separate threads,
// give what each gives alone
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
#include "engine/engine.h"
using propan2ol::Engine;
static std::string phonemize( Engine* s, const std::string& text ) {
	const void* p = text.c_str();
	std::string out;
	while ( p ) {
		const char* r = s->text_to_phonemes( &p, ( '_' << 8 ) | 0x02 );
		if ( r ) {
			out += r;
			out += ' ';
		}
	}
	return out;
}
int main() {
	const char* texts[] = { "Hello world, this is a test.", "Bonjour le monde, ça va? 123", "The 3 quick brown foxes in 1999!",
							"Guten Tag, wie geht es dir?" };
	auto solo = [&]( const char* lang, const std::string& t ) {
		auto holder = Engine::create();
		Engine* s = holder.get();
		s->set_language( lang );
		return phonemize( s, t );
	};
	const std::string en[4] = { solo( "en-us", texts[0] ), solo( "en-us", texts[1] ), solo( "en-us", texts[2] ), solo( "en-us", texts[3] ) };
	const std::string fr[4] = { solo( "fr-fr", texts[0] ), solo( "fr-fr", texts[1] ), solo( "fr-fr", texts[2] ), solo( "fr-fr", texts[3] ) };
	int bad = 0;
	auto a_holder = Engine::create();
	auto b_holder = Engine::create();
	Engine* a = a_holder.get();
	Engine* b = b_holder.get();
	a->set_language( "en-us" );
	b->set_language( "fr-fr" );
	for ( int round = 0; round < 3; ++round ) {
		for ( int i = 0; i < 4; ++i ) {
			bad += phonemize( a, texts[i] ) != en[i];
			bad += phonemize( b, texts[i] ) != fr[i];
		}
	}
	std::printf( "interleaved: %s\n", bad ? "FAIL" : "ok" );
	std::vector<std::thread> threads;
	std::vector<int> fails( 4 );
	for ( int t = 0; t < 4; ++t ) {
		threads.emplace_back( [&, t] {
			auto holder = Engine::create();
			Engine* s = holder.get();
			const bool french = t & 1;
			s->set_language( french ? "fr-fr" : "en-us" );
			for ( int round = 0; round < 200; ++round ) {
				for ( int i = 0; i < 4; ++i ) {
					fails[t] += phonemize( s, texts[i] ) != ( french ? fr[i] : en[i] );
				}
			}
		} );
	}
	for ( auto& t : threads ) {
		t.join();
	}
	int tf = 0;
	for ( int f : fails ) {
		tf += f;
	}
	std::printf( "threads: %s\n", tf ? "FAIL" : "ok" );
	return bad || tf;
}
