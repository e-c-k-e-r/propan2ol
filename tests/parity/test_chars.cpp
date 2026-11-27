// every code point's class and case map against the platform's glibc C.UTF-8 (Linux dev check; the tables are the source on
// other platforms)
#include "../../src/chars.h"
#include <clocale>
#include <cstdio>
#include <cwctype>

int main() {
	if ( !std::setlocale( LC_CTYPE, "C.UTF-8" ) ) {
		std::puts( "no C.UTF-8" );
		return 2;
	}
	using namespace propan2ol::chars;
	int bad = 0;
	for ( uint32_t c = 0; c < 0x110000; ++c ) {
		const wint_t w = c;
		const bool ok = is_alpha( c ) == !!iswalpha( w ) && is_alnum( c ) == !!iswalnum( w ) && is_space( c ) == !!iswspace( w ) &&
						is_upper( c ) == !!iswupper( w ) && is_lower( c ) == !!iswlower( w ) && is_digit( c ) == !!iswdigit( w ) &&
						is_punct( c ) == !!iswpunct( w ) && to_lower( c ) == towlower( w ) && to_upper( c ) == towupper( w );
		if ( !ok && ++bad < 10 ) {
			std::printf( "mismatch at U+%04X\n", c );
		}
	}
	std::printf( "%s (%d mismatches)\n", bad ? "FAIL" : "ok", bad );
	return bad != 0;
}
