// See text_decoder.h. From espeak-ng's encoding.c (string_decoder_getc_utf_8, text_decoder_*).
#include "text_decoder.h"
#include <cstring>

namespace propan2ol {

void TextDecoder::reset( const char* utf8 ) {
	current_ = reinterpret_cast<const uint8_t*>( utf8 );
	end_ = utf8 ? current_ + std::strlen( utf8 ) + 1 : current_; // the terminating NUL is input too
}

uint32_t TextDecoder::get() {
	if ( current_ == end_ ) {
		return 0;
	} // upstream's null decoder, for an absent string
	const uint8_t c = *current_++;
	if ( c < 0x80 ) {
		return c;
	}
	if ( c < 0xc0 ) {
		return 0xfffd;
	} // a tail byte on its own
	const int tails = c < 0xe0 ? 1 : c < 0xf0 ? 2 : 3;
	if ( current_ + tails >= end_ ) {
		current_ = end_;
		return 0xfffd;
	} // truncated
	uint32_t ret = c & ( tails == 1 ? 0x1f : 0x0f );
	for ( int i = 0; i < tails; ++i ) {
		const uint8_t t = *current_++;
		if ( ( t & 0xc0 ) != 0x80 ) {
			--current_;
			return 0xfffd;
		} // a tail byte must be 10xxxxxx
		ret = ( ret << 6 ) + ( t & 0x3f );
	}
	if ( tails == 2 && ret == 0xfffd ) {
		return 0x1a;
	}
	if ( tails == 3 && ret > 0x10ffff ) {
		return 0xfffd;
	}
	return ret;
}

uint32_t TextDecoder::peek() {
	if ( current_ == end_ ) {
		return 0;
	}
	const uint8_t* saved = current_;
	const uint32_t c = get();
	current_ = saved;
	return c;
}

} // namespace propan2ol
