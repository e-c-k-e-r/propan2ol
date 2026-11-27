// The compiled espeak-ng data (phontab, phonindex, phondata, <lang>_dict) embedded in the library; see cmake/embed_data.cmake.
#pragma once
#include <cstddef>

namespace propan2ol {

struct Blob {
	const unsigned char* data;
	size_t size;
};

// The embedded file called `name`, or false.
bool find_blob( const char* name, Blob& out );

} // namespace propan2ol

extern "C++" {
// C-callable form for the vendored sources: 0 and the bytes if found, else -1.
inline int p2o_find_data( const char* name, const unsigned char** data, size_t* size ) {
	propan2ol::Blob b;
	if ( !propan2ol::find_blob( name, b ) ) {
		return -1;
	}
	*data = b.data;
	*size = b.size;
	return 0;
}
}
