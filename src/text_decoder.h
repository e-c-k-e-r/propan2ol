// The UTF-8 text decoder of espeak-ng's encoding.c, as a class: one string's code points, one at a time, with look-ahead.
//
// Only UTF-8 is kept (the phonemizer always passes UTF-8). Behaviour on malformed input is upstream's, quirks included: the string's
// terminating NUL is part of the input, a truncated sequence ends the input as U+FFFD, a bare tail byte is U+FFFD, and a 3-byte sequence
// that decodes to U+FFFD becomes U+001A ("I umlaut a half").
#pragma once
#include <cstdint>

namespace propan2ol {

class TextDecoder {
  public:
	// Start decoding `utf8` (NUL-terminated; it must outlive the decoding). nullptr: nothing to decode.
	void reset( const char* utf8 );
	bool eof() const { return current_ == end_; }
	uint32_t get();	 // the next character (0 at the end); U+FFFD for bad input
	uint32_t peek(); // the next character, without consuming it
	// where decoding stands, as a pointer into the string given to reset(); nullptr at the end
	const char* position() const { return eof() ? nullptr : reinterpret_cast<const char*>( current_ ); }

  private:
	const uint8_t* current_ = nullptr;
	const uint8_t* end_ = nullptr;
};

} // namespace propan2ol
