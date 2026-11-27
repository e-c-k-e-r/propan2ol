// The phonemizer layer: phonemizer 3.3's EspeakBackend (phonemizer/backend/espeak/espeak.py, base.py, language_switch.py and
// punctuation.py) over the core, behaviour for behaviour, quirks included. Text is handled as code points; Python's \s and strip()
// are str.isspace, which is not iswspace.
#include "engine/engine.h"
#include "chars.h"
#include "text.h"
#include <propan2ol/propan2ol.h>
#include <algorithm>
#include <cstring>

namespace propan2ol {

struct Phonemizer::State {
	std::unique_ptr<Engine> engine;
};

namespace {

using u32 = std::u32string;
using text::decode_utf8;
using text::encode_utf8;

constexpr std::u32string_view DEFAULT_MARKS = U";:,.!?¡¿—…\"«»“”(){}[]";
constexpr char32_t TIE_DEFAULT = 0x361; // the combining double inverted breve phonemizer's tie=True means

// str.isspace()
bool is_space( char32_t c ) {
	return ( c >= 0x09 && c <= 0x0d ) || ( c >= 0x1c && c <= 0x20 ) || c == 0x85 || c == 0xa0 || c == 0x1680 || ( c >= 0x2000 && c <= 0x200a ) ||
		   c == 0x2028 || c == 0x2029 || c == 0x202f || c == 0x205f || c == 0x3000;
}

u32 strip( const u32& s ) {
	size_t a = 0, b = s.size();
	while ( a < b && is_space( s[a] ) ) {
		++a;
	}
	while ( b > a && is_space( s[b - 1] ) ) {
		--b;
	}
	return s.substr( a, b - a );
}

u32 replace_all( const u32& s, const u32& from, const u32& to ) { // str.replace
	if ( from.empty() ) {
		return s;
	}
	u32 out;
	size_t i = 0;
	for ( size_t j; ( j = s.find( from, i ) ) != u32::npos; i = j + from.size() ) {
		out += s.substr( i, j - i ) + to;
	}
	return out + s.substr( i );
}

bool ends_with( const u32& s, const u32& p ) { return s.size() >= p.size() && s.compare( s.size() - p.size(), p.size(), p ) == 0; }
bool starts_with( const u32& s, const u32& p ) { return s.size() >= p.size() && s.compare( 0, p.size(), p ) == 0; }

// ---- punctuation ----

struct Mark {
	int index;
	u32 mark;
	char position; // B(eginning), I(nside), E(nd), A(lone)
};

// The matches of (\s*[marks]+\s*)+, left to right: the maximal stretches of whitespace and marks that hold a mark.
std::vector<std::pair<size_t, size_t>> mark_runs( const u32& s, const u32& marks ) {
	const auto is_mark = [&]( char32_t c ) { return marks.find( c ) != u32::npos; };
	std::vector<std::pair<size_t, size_t>> out;
	for ( size_t i = 0; i < s.size(); ) {
		if ( !is_space( s[i] ) && !is_mark( s[i] ) ) {
			++i;
			continue;
		}
		size_t j = i;
		bool has = false;
		while ( j < s.size() && ( is_space( s[j] ) || is_mark( s[j] ) ) ) {
			has |= is_mark( s[j] );
			++j;
		}
		if ( has ) {
			out.emplace_back( i, j );
		}
		i = j;
	}
	return out;
}

// Punctuation.remove for one line: every run becomes a space, then strip().
u32 remove_marks( const u32& line, const u32& marks ) {
	u32 out;
	size_t last = 0;
	for ( const auto& [a, b] : mark_runs( line, marks ) ) {
		out += line.substr( last, a - last ) + U" ";
		last = b;
	}
	return strip( out + line.substr( last ) );
}

// Punctuation._preserve_line: the pieces between the runs go to `chunks`, the runs to `marks`.
void preserve_line( const u32& line0, int num, const u32& marks_set, std::vector<u32>& chunks, std::vector<Mark>& marks ) {
	u32 line = line0;
	const auto runs = mark_runs( line, marks_set );
	if ( runs.empty() ) {
		chunks.push_back( line );
		return;
	}
	if ( runs.size() == 1 && runs[0].first == 0 && runs[0].second == line.size() ) {
		marks.push_back( { num, line, 'A' } );
		return;
	}

	std::vector<Mark> found;
	for ( size_t k = 0; k < runs.size(); ++k ) {
		const u32 g = line.substr( runs[k].first, runs[k].second - runs[k].first );
		char position = 'I';
		if ( k == 0 && starts_with( line, g ) ) {
			position = 'B';
		} else if ( k + 1 == runs.size() && ends_with( line, g ) ) {
			position = 'E';
		}
		found.push_back( { num, g, position } );
	}
	for ( const Mark& m : found ) {
		// line.split( mark ): the text before its first occurrence, and the rest
		const size_t at = line.find( m.mark );
		if ( at == u32::npos ) {
			chunks.push_back( line );
			line.clear();
			continue;
		}
		chunks.push_back( line.substr( 0, at ) );
		line = line.substr( at + m.mark.size() );
	}
	chunks.push_back( line );
	marks.insert( marks.end(), found.begin(), found.end() );
}

// Punctuation.restore
std::vector<u32> restore( std::vector<u32> text, const std::vector<Mark>& marks, const u32& sep_word, bool strip_output ) {
	std::vector<u32> out;
	int pos = 0;
	size_t mi = 0, ti = 0;
	const auto spaces_to_sep = [&]( const u32& m ) { return replace_all( m, U" ", sep_word ); }; // re.sub( ' ', sep.word, mark )
	while ( ti < text.size() || mi < marks.size() ) {
		if ( mi == marks.size() ) {
			for ( ; ti < text.size(); ++ti ) {
				u32 line = text[ti];
				if ( !strip_output && !sep_word.empty() && !ends_with( line, sep_word ) ) {
					line += sep_word;
				}
				out.push_back( line );
			}
		} else if ( ti == text.size() ) {
			u32 all;
			for ( ; mi < marks.size(); ++mi ) {
				all += marks[mi].mark;
			}
			out.push_back( spaces_to_sep( all ) );
		} else {
			const Mark& cur = marks[mi];
			if ( cur.index == pos ) {
				const u32 mark = spaces_to_sep( cur.mark );
				++mi;
				if ( !sep_word.empty() && ends_with( text[ti], sep_word ) ) {
					text[ti].resize( text[ti].size() - sep_word.size() );
				}
				const u32 tail = strip_output || ends_with( mark, sep_word ) ? U"" : sep_word;
				if ( cur.position == 'B' ) {
					text[ti] = mark + text[ti];
				} else if ( cur.position == 'E' ) {
					out.push_back( text[ti] + mark + tail );
					++ti;
					++pos;
				} else if ( cur.position == 'A' ) {
					out.push_back( mark + tail );
					++pos;
				} else if ( ti + 1 == text.size() ) {
					text[ti] += mark;
				} else {
					const u32 first = text[ti];
					++ti;
					text[ti] = first + mark + text[ti];
				}
			} else {
				out.push_back( text[ti] );
				++ti;
				++pos;
			}
		}
	}
	return out;
}

// ---- language switches ----

// The matches of \(.+?\) in `line` as [begin, end) pairs: '.' is any character but a newline, and lazy.
std::vector<std::pair<size_t, size_t>> switch_flags( const u32& line ) {
	std::vector<std::pair<size_t, size_t>> out;
	for ( size_t i = 0; i < line.size(); ) {
		if ( line[i] == U'(' && i + 1 < line.size() && line[i + 1] != U'\n' ) {
			const size_t close = line.find( U')', i + 2 );
			if ( close != u32::npos && line.find( U'\n', i + 1 ) > close ) {
				out.emplace_back( i, close + 1 );
				i = close + 1;
				continue;
			}
		}
		++i;
	}
	return out;
}

// ---- the espeak backend ----

u32 process_stress( const u32& word, bool with_stress ) {
	if ( with_stress ) {
		return word;
	}
	u32 out; // re.sub( r"[ˈˌ'-]+", '', word )
	for ( const char32_t c : word ) {
		if ( c != U'ˈ' && c != U'ˌ' && c != U'\'' && c != U'-' ) {
			out += c;
		}
	}
	return out;
}

// EspeakBackend._postprocess_line: one espeak output line to its final form.
u32 postprocess_line( u32 line, const Options& o, const u32& tie, const u32& sep_word, const u32& sep_phone, bool strip_output ) {
	line = replace_all( replace_all( strip( line ), U"\n", U" " ), U"  ", U" " );

	u32 collapsed; // re.sub( r'_+', '_', line )
	for ( const char32_t c : line ) {
		if ( !( c == U'_' && !collapsed.empty() && collapsed.back() == U'_' ) ) {
			collapsed += c;
		}
	}
	line = replace_all( collapsed, U"_ ", U" " );

	const auto flags = switch_flags( line );
	if ( !flags.empty() ) {
		if ( o.language_switch == LanguageSwitch::RemoveUtterance ) {
			return U"";
		}
		if ( o.language_switch == LanguageSwitch::RemoveFlags ) {
			u32 kept;
			size_t last = 0;
			for ( const auto& [a, b] : flags ) {
				kept += line.substr( last, a - last );
				last = b;
			}
			line = kept + line.substr( last );
		}
	}
	if ( line.empty() ) {
		return U"";
	}

	u32 out;
	size_t i = 0;
	for ( bool more = true; more; ) { // line.split( ' ' )
		const size_t j = line.find( U' ', i );
		more = j != u32::npos;
		u32 word = process_stress( strip( line.substr( i, more ? j - i : u32::npos ) ), o.with_stress );
		if ( !strip_output && tie.empty() ) {
			word += U'_';
		}
		// _process_tie
		if ( !tie.empty() && tie != u32( 1, TIE_DEFAULT ) ) {
			word = replace_all( word, u32( 1, TIE_DEFAULT ), tie );
		} else {
			word = replace_all( word, U"_", sep_phone );
		}
		out += word + sep_word;
		i = more ? j + 1 : i;
	}
	if ( strip_output && !sep_word.empty() ) {
		out.resize( out.size() - sep_word.size() );
	}
	return out;
}

} // namespace

// ---- Phonemizer ----

Phonemizer::~Phonemizer() = default;

std::vector<std::string> Phonemizer::languages( const std::string& profile ) {
	std::vector<std::string> out;
	if ( profile != DEFAULT_PROFILE ) {
		return out;
	}
	for ( const char* const* c = Engine::languages(); *c; ++c ) {
		out.emplace_back( *c );
	}
	return out;
}

std::unique_ptr<Phonemizer> Phonemizer::create( const std::string& profile, const std::string& language, std::string* error ) {
	const auto fail = [&]( const std::string& message ) {
		if ( error ) {
			*error = message;
		}
		return std::unique_ptr<Phonemizer>();
	};
	if ( profile != DEFAULT_PROFILE ) {
		return fail( "unknown profile \"" + profile + "\" (the profiles are: " + DEFAULT_PROFILE + ")" );
	}

	std::unique_ptr<Phonemizer> p( new Phonemizer );
	p->state_ = std::make_unique<State>();
	p->state_->engine = Engine::create();
	if ( !p->state_->engine ) {
		return fail( "the embedded data is invalid" );
	}
	if ( !p->state_->engine->set_language( language.c_str() ) ) {
		std::string known;
		for ( const std::string& l : languages( profile ) ) {
			known += ( known.empty() ? "" : ", " ) + l;
		}
		return fail( "language \"" + language + "\" is not available in profile " + profile + " (available: " + known + ")" );
	}
	p->language_ = language;
	p->profile_ = profile;
	return p;
}

std::string Phonemizer::raw( const std::string& text ) const {
	// wrapper.py text_to_phonemes: a clause per call, the non-empty ones joined by spaces
	const void* p = text.c_str();
	std::string out;
	bool first = true;
	while ( p ) {
		const char* ph = state_->engine->text_to_phonemes( &p, ( '_' << 8 ) | 0x02 );
		if ( ph && *ph ) {
			if ( !first ) {
				out += ' ';
			}
			out += ph;
			first = false;
		}
	}
	return out;
}

std::vector<std::string> Phonemizer::phonemize( const std::vector<std::string>& lines, const Options& o ) const {
	const u32 sep_word = decode_utf8( o.separator.word ), sep_phone = decode_utf8( o.separator.phone );
	const u32 tie = decode_utf8( o.tie );
	const u32 marks_set = o.punctuation_marks.empty() ? u32( DEFAULT_MARKS ) : decode_utf8( o.punctuation_marks );

	// preprocess: remove the punctuation, or keep the pieces and marks for restoring
	std::vector<u32> text, phonemized;
	std::vector<Mark> marks;
	if ( o.preserve_punctuation ) {
		std::vector<u32> chunks;
		for ( size_t num = 0; num < lines.size(); ++num ) {
			preserve_line( decode_utf8( lines[num] ), static_cast<int>( num ), marks_set, chunks, marks );
		}
		for ( u32& c : chunks ) {
			if ( !c.empty() ) {
				text.push_back( std::move( c ) );
			}
		}
	} else {
		for ( const std::string& l : lines ) {
			text.push_back( remove_marks( decode_utf8( l ), marks_set ) );
		}
	}

	// _phonemize_aux: espeak (with the tie, if any), then _postprocess_line
	const int mode = tie.empty() ? ( '_' << 8 ) | 0x02 : 0x02 | ( 0x01 << 7 ) | ( TIE_DEFAULT << 8 );
	for ( const u32& line : text ) {
		const std::string utf8 = encode_utf8( line );
		const void* p = utf8.c_str();
		std::string out;
		bool first = true;
		while ( p ) {
			const char* ph = state_->engine->text_to_phonemes( &p, mode );
			if ( ph && *ph ) {
				if ( !first ) {
					out += ' ';
				}
				out += ph;
				first = false;
			}
		}
		phonemized.push_back( postprocess_line( decode_utf8( out ), o, tie, sep_word, sep_phone, o.strip ) );
	}

	if ( o.preserve_punctuation ) {
		phonemized = restore( std::move( phonemized ), marks, sep_word, o.strip );
	}

	std::vector<std::string> result;
	for ( const u32& l : phonemized ) {
		result.push_back( encode_utf8( l ) );
	}
	return result;
}

std::string Phonemizer::phonemize( const std::string& text, const Options& options ) const {
	const auto lines = phonemize( std::vector<std::string>{ text }, options );
	return lines.empty() ? std::string() : lines.front();
}

} // namespace propan2ol
