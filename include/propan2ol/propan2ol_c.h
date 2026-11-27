/* propan2ol's C ABI over the C++ API (propan2ol.h): for bindings and shared-library boundaries. Strings are UTF-8, NUL-terminated.
 * Functions return NULL or a negative value on failure; p2o_last_error() describes the last failure on the calling thread. */
#ifndef PROPAN2OL_C_H
#define PROPAN2OL_C_H

#include <stddef.h>

#if defined( _WIN32 )
#if defined( PROPAN2OL_BUILD_SHARED )
#define P2O_API __declspec( dllexport )
#else
#define P2O_API
#endif
#else
#define P2O_API __attribute__( ( visibility( "default" ) ) )
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct p2o_phonemizer p2o_phonemizer;

/* What to do with an utterance espeak switched language in (see LanguageSwitch in propan2ol.h). */
enum { P2O_KEEP_FLAGS = 0, P2O_REMOVE_FLAGS = 1, P2O_REMOVE_UTTERANCE = 2 };

/* Set struct_size to sizeof( p2o_options ) (or start from p2o_options_default()) so fields can be added later. NULL strings mean the
 * default: " " for the word separator, none for the others, phonemizer's marks for punctuation_marks, no tie. */
typedef struct p2o_options {
	size_t struct_size;
	int preserve_punctuation;
	int with_stress;
	int strip;
	int language_switch; /* P2O_* */
	const char* punctuation_marks;
	const char* tie; /* a single character, or NULL / "" for no tie */
	const char* word_separator;
	const char* syllable_separator;
	const char* phone_separator;
} p2o_options;

P2O_API p2o_options p2o_options_default( void );

/* profile NULL: the default ("espeak-1.52"). */
P2O_API p2o_phonemizer* p2o_create( const char* profile, const char* language );
P2O_API void p2o_destroy( p2o_phonemizer* phonemizer );

/* The phonemized text as a newly allocated string: free it with p2o_free(). options NULL: the defaults. */
P2O_API char* p2o_phonemize( const p2o_phonemizer* phonemizer, const char* text, const p2o_options* options );
P2O_API void p2o_free( void* ptr );

/* The same for several lines at once, as phonemizer's phonemize( [lines] ): the outputs can be fewer or more than the inputs only through
 * what the punctuation handling does; free the result with p2o_free_lines( lines, count ). */
P2O_API char** p2o_phonemize_lines( const p2o_phonemizer* phonemizer, const char* const* lines, size_t n_lines, const p2o_options* options,
									size_t* n_out );
P2O_API void p2o_free_lines( char** lines, size_t n );

/* The language codes of a profile (NULL: the default), as a newly allocated '\n'-separated string. */
P2O_API char* p2o_languages( const char* profile );

P2O_API const char* p2o_last_error( void );

#ifdef __cplusplus
}
#endif

#endif
