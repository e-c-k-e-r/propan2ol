// UTF-8 coding and character-class helpers (espeak-ng's common.c, rewritten).
#pragma once

namespace propan2ol::text {

// Reads the character at buf into c and returns the bytes used. Skips tail bytes first (forwards, or backwards when moving back).
int utf8_in( int& c, const char* buf, bool backwards = false );
// Writes c as UTF-8 and returns the bytes written (1-4).
int utf8_out( unsigned c, char* buf );

bool is_alpha( unsigned c ); // iswalpha plus in-word marks of several scripts
int is_bracket( int c );	 // 0, or the position + 1 in the bracket list (a range of general punctuation counts as 1)
bool is_digit09( unsigned c );
bool is_digit( unsigned c ); // iswdigit plus Devanagari digits
bool is_space( unsigned c ); // iswspace plus box drawing and the specials block; false for NUL
bool is_space_ascii( unsigned c );
bool is_all_zero( const char* s, int size );

} // namespace propan2ol::text

#include <string>
#include <string_view>

namespace propan2ol::text {

// UTF-8 to code points; a malformed byte becomes U+FFFD.
std::u32string decode_utf8( std::string_view s );
std::string encode_utf8( std::u32string_view s );

} // namespace propan2ol::text
