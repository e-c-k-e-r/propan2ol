// Character classification with espeak-ng's behaviour on Linux: the glibc C.UTF-8 classes (the iswalpha / towlower family
// espeak calls after setlocale( LC_CTYPE, "C.UTF-8" )) and ucd-tools' case maps, from fixed tables. The platform's wctype is
// never used.
#pragma once
#include <cstdint>

namespace propan2ol::chars {

bool is_alpha( uint32_t c );
bool is_alnum( uint32_t c );
bool is_space( uint32_t c );
bool is_upper( uint32_t c );
bool is_lower( uint32_t c );
bool is_digit( uint32_t c );
bool is_punct( uint32_t c );
uint32_t to_lower( uint32_t c );	 // towlower
uint32_t to_upper( uint32_t c );	 // towupper
uint32_t ucd_to_lower( uint32_t c ); // ucd_tolower
uint32_t ucd_to_upper( uint32_t c ); // ucd_toupper
int clause_type( uint32_t c );		 // clause_type_from_codepoint (readclause.c)

} // namespace propan2ol::chars
