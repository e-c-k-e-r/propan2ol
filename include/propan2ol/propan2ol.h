// propan2ol: text to IPA, a standalone port of the phonemizing part of espeak-ng, with the processing of phonemizer 3.3's
// espeak backend on top.
//
// A Phonemizer is one language under one profile. A profile is a named, versioned behaviour: "espeak-1.52" (the default, and
// frozen) reproduces espeak-ng 1.52.0 plus phonemizer 3.3 character for character. Instances share nothing, so they can be
// used from different threads at once; one instance is not safe to call from two threads at the same time.
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace propan2ol {

inline constexpr const char* DEFAULT_PROFILE = "espeak-1.52";

// What to do with an utterance in which espeak switched language partway (an English word in French text comes out as
// "(en)...(fr)" flags).
enum class LanguageSwitch {
	KeepFlags,		 // leave the utterance as it is
	RemoveFlags,	 // drop the "(xx)" flags, keep the words
	RemoveUtterance, // drop the whole utterance (it comes out empty)
};

// Boundary tokens in the output. Empty = none; the non-empty ones must differ from each other.
struct Separator {
	std::string word = " ";
	std::string syllable;
	std::string phone;
};

struct Options {
	bool preserve_punctuation = false; // put the punctuation marks back around the phonemized pieces
	std::string punctuation_marks;	   // single-character marks; empty = phonemizer's default set
	bool with_stress = false;		   // keep the stress marks (ˈ ˌ ' -); else they are removed
	std::string tie;				   // empty: no tie. Else a single character joining the characters of a phoneme (phonemizer's
									   // tie=True is "͡"); the phone separator is then ignored
	LanguageSwitch language_switch = LanguageSwitch::KeepFlags;
	Separator separator;
	bool strip = false; // drop the last word / phone separator of each utterance
};

class Phonemizer {
  public:
	// nullptr, with `error` set if given, for an unknown profile or a language that isn't available
	static std::unique_ptr<Phonemizer> create( const std::string& profile, const std::string& language, std::string* error = nullptr );
	// the language codes of a profile (espeak's first language of each voice)
	static std::vector<std::string> languages( const std::string& profile = DEFAULT_PROFILE );

	~Phonemizer();
	Phonemizer( const Phonemizer& ) = delete;
	Phonemizer& operator=( const Phonemizer& ) = delete;

	// phonemizer's EspeakBackend.phonemize: one output per input line
	std::vector<std::string> phonemize( const std::vector<std::string>& lines, const Options& options = {} ) const;
	// one text; the same as phonemize( { text } )[0]
	std::string phonemize( const std::string& text, const Options& options = {} ) const;

	// The core on its own: espeak_TextToPhonemes over every clause of `text`, joined by spaces, IPA with '_' between phonemes
	std::string raw( const std::string& text ) const;

	const std::string& language() const { return language_; }
	const std::string& profile() const { return profile_; }

  private:
	Phonemizer() = default;
	struct State;
	std::unique_ptr<State> state_;
	std::string language_, profile_;
};

} // namespace propan2ol
