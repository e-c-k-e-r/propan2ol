#include "chars.h"
#include "ctype_tables.h"
#include "ucd_tables.h"
#include <algorithm>

namespace propan2ol::chars {

namespace {

bool in_ranges( const ctype_tables::CodeRange* r, size_t n, uint32_t c ) {
	const auto* end = r + n;
	const auto* it = std::upper_bound( r, end, c, []( uint32_t v, const ctype_tables::CodeRange& x ) { return v < x.first; } );
	return it != r && c <= ( it - 1 )->last;
}

template <class Pair> uint32_t map_case( const Pair* t, size_t n, uint32_t c ) {
	const auto* end = t + n;
	const auto* it = std::lower_bound( t, end, c, []( const Pair& x, uint32_t v ) { return x.from < v; } );
	return it != end && it->from == c ? it->to : c;
}

} // namespace

bool is_alpha( uint32_t c ) { return in_ranges( ctype_tables::alpha, ctype_tables::alpha_count, c ); }
bool is_alnum( uint32_t c ) { return in_ranges( ctype_tables::alnum, ctype_tables::alnum_count, c ); }
bool is_space( uint32_t c ) { return in_ranges( ctype_tables::space, ctype_tables::space_count, c ); }
bool is_upper( uint32_t c ) { return in_ranges( ctype_tables::upper, ctype_tables::upper_count, c ); }
bool is_lower( uint32_t c ) { return in_ranges( ctype_tables::lower, ctype_tables::lower_count, c ); }
bool is_digit( uint32_t c ) { return in_ranges( ctype_tables::digit, ctype_tables::digit_count, c ); }
bool is_punct( uint32_t c ) { return in_ranges( ctype_tables::punct, ctype_tables::punct_count, c ); }
uint32_t to_lower( uint32_t c ) { return map_case( ctype_tables::to_lower, ctype_tables::to_lower_count, c ); }
uint32_t to_upper( uint32_t c ) { return map_case( ctype_tables::to_upper, ctype_tables::to_upper_count, c ); }
uint32_t ucd_to_lower( uint32_t c ) { return map_case( ucd_tables::ucd_lower, ucd_tables::ucd_lower_count, c ); }
uint32_t ucd_to_upper( uint32_t c ) { return map_case( ucd_tables::ucd_upper, ucd_tables::ucd_upper_count, c ); }

int clause_type( uint32_t c ) {
	const auto* t = ucd_tables::clause_types;
	const auto* end = t + ucd_tables::clause_types_count;
	const auto* it = std::lower_bound( t, end, c, []( const ucd_tables::ClauseType& x, uint32_t v ) { return x.cp < v; } );
	return it != end && it->cp == c ? it->type : 0x4000; // CLAUSE_NONE
}

} // namespace propan2ol::chars
