// UTF-8 coding and character-class helpers, from espeak-ng's common.c (utf8_in2, utf8_out, IsAlpha, IsBracket, IsDigit, IsSpace,
// towlower2 and friends). The behaviour is upstream's, including its quirks: utf8_in2 skips tail bytes before a character and
// stops at a NUL inside a truncated sequence, utf8_out writes a space for anything past U+10FFFF, IsSpace treats box-drawing and
// the specials block as space. Dropped as unused on this path: Read4Bytes, StringToWord, espeak_rand / espeak_srand.
// The C names stay (extern "C") for the vendored files that haven't been rewritten yet.
#include "chars.h"
#include "text.h"
#include <cstring>

namespace propan2ol::text {

namespace {

constexpr char32_t BRACKETS[] = {
	U'(',		   U')',   U'[', U']', U'{', U'}', U'<', U'>', U'"', U'\'', U'`', 0xab, 0xbb, // double angle brackets
	0x300a,		   0x300b,																	  // double angle brackets (ideograph)
	0xe000 + U'<',																			  // private usage area
};

} // namespace

int utf8_in( int& c, const char* buf, bool backwards ) {
	// the start of the next / previous character: skip over tail bytes
	while ( ( *buf & 0xc0 ) == 0x80 ) {
		backwards ? --buf : ++buf;
	}
	static constexpr unsigned char MASK[4] = { 0xff, 0x1f, 0x0f, 0x07 };
	int c1 = static_cast<unsigned char>( *buf++ );
	int n_bytes = 0;
	if ( c1 & 0x80 ) {
		if ( ( c1 & 0xe0 ) == 0xc0 ) {
			n_bytes = 1;
		} else if ( ( c1 & 0xf0 ) == 0xe0 ) {
			n_bytes = 2;
		} else if ( ( c1 & 0xf8 ) == 0xf0 ) {
			n_bytes = 3;
		}
		c1 &= MASK[n_bytes];
		int i = 0;
		for ( ; i < n_bytes; ++i ) {
			if ( !*buf ) {
				break;
			} // truncated
			c1 = ( c1 << 6 ) + ( *buf++ & 0x3f );
		}
		n_bytes = i;
	}
	c = c1;
	return n_bytes + 1;
}

int utf8_out( unsigned c, char* buf ) {
	if ( c < 0x80 ) {
		buf[0] = static_cast<char>( c );
		return 1;
	}
	if ( c >= 0x110000 ) {
		buf[0] = ' ';
		return 1;
	} // out of range
	static constexpr unsigned char LEAD[4] = { 0, 0xc0, 0xe0, 0xf0 };
	const int n_bytes = c < 0x800 ? 1 : c < 0x10000 ? 2 : 3;
	int shift = 6 * n_bytes;
	buf[0] = static_cast<char>( LEAD[n_bytes] | ( c >> shift ) );
	for ( int j = 0; j < n_bytes; ++j ) {
		shift -= 6;
		buf[j + 1] = static_cast<char>( 0x80 + ( ( c >> shift ) & 0x3f ) );
	}
	return n_bytes + 1;
}

bool is_alpha( unsigned c ) {
	// iswalpha plus some in-word symbols
	if ( chars::is_alpha( c ) ) {
		return true;
	}
	if ( c < 0x300 ) {
		return false;
	}
	if ( c >= 0x901 && c <= 0xdf7 ) { // Indic scripts: Devanagari, Tamil, etc
		return ( c & 0x7f ) < 0x64 || c == 0xa70 || c == 0xa71 /* Gurmukhi tippi, addak */ || ( c >= 0xd7a && c <= 0xd7f ) /* Malayalam chillu */;
	}
	return ( c >= 0x5b0 && c <= 0x5c2 )								   // Hebrew vowel marks
		   || c == 0x605 || c == 0x670 || ( c >= 0x64b && c <= 0x65e ) // Arabic vowel marks
		   || ( c >= 0x300 && c <= 0x36f )							   // combining accents
		   || ( c >= 0xf40 && c <= 0xfbc )							   // Tibetan
		   || ( c >= 0x1100 && c <= 0x11ff )						   // Korean jamo
		   || ( c >= 0x2800 && c <= 0x28ff )						   // braille
		   || ( c > 0x3040 && c <= 0xa700 );						   // Chinese / Japanese (upstream's guard against a broken iswalpha)
}

int is_bracket( int c ) {
	if ( c >= 0x2014 && c <= 0x201f ) {
		return 1;
	}
	for ( size_t i = 0; i < sizeof( BRACKETS ) / sizeof( BRACKETS[0] ); ++i ) {
		if ( BRACKETS[i] == static_cast<char32_t>( c ) ) {
			return static_cast<int>( i ) + 1;
		}
	}
	return 0;
}

bool is_digit09( unsigned c ) { return c >= '0' && c <= '9'; }
bool is_digit( unsigned c ) { return chars::is_digit( c ) || ( c >= 0x966 && c <= 0x96f ); }

bool is_space( unsigned c ) {
	if ( c == 0 ) {
		return false;
	}
	if ( c >= 0x2500 && c < 0x25a0 ) {
		return true;
	} // box drawing
	if ( c >= 0xfff9 && c <= 0xffff ) {
		return true;
	} // unicode specials
	return chars::is_space( c );
}

bool is_space_ascii( unsigned c ) { return ( c & 0xff ) != 0 && c <= ' '; }

bool is_all_zero( const char* s, int size ) { return *s == 0 && std::memcmp( s, s + 1, size - 1 ) == 0; }

std::u32string decode_utf8( std::string_view s ) {
	std::u32string out;
	for ( size_t i = 0; i < s.size(); ) {
		const unsigned char c = static_cast<unsigned char>( s[i] );
		const int n = c < 0x80 ? 1 : ( c >> 5 ) == 6 ? 2 : ( c >> 4 ) == 14 ? 3 : ( c >> 3 ) == 30 ? 4 : 0;
		char32_t cp = n == 1 ? c : n == 2 ? c & 0x1f : n == 3 ? c & 0x0f : n == 4 ? c & 0x07 : 0xfffd;
		size_t used = n == 0 ? 1 : n;
		if ( n > 1 && i + n <= s.size() ) {
			for ( int k = 1; k < n; ++k ) {
				cp = ( cp << 6 ) | ( static_cast<unsigned char>( s[i + k] ) & 0x3f );
			}
		} else if ( n > 1 ) {
			cp = 0xfffd;
			used = 1;
		}
		out += cp;
		i += used;
	}
	return out;
}

std::string encode_utf8( std::u32string_view s ) {
	std::string out;
	char buf[4];
	for ( const char32_t c : s ) {
		const int n = utf8_out( static_cast<unsigned>( c ), buf );
		out.append( buf, n );
	}
	return out;
}

} // namespace propan2ol::text
