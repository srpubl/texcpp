#pragma once

#include <cstring>
#include <string_view>

using namespace std::literals;

namespace config {

constexpr auto banner = "This is a C++ reimplementation of TANGLE, Version 4.6"sv;
constexpr size_t buf_size = 100;  /// maximum length of input line
constexpr size_t hash_size    = 353;    /// should be prime
constexpr size_t max_bytes
    = 2 * 45000;  /// number of bytes in identifiers, strings, and module names; must be < 65536
constexpr size_t max_toks     = 65000;  /// number of bytes in compressed Pascal code; must be < 65536
constexpr size_t max_names    = 4000;   /// number of identifiers, strings, module names; must be < 10240
constexpr size_t max_texts    = 2000;   /// number of replacement texts, must be < 10240
constexpr size_t longest_name = 400;    /// module names shouldn’t be longer than this
constexpr size_t line_length  = 72;     /// lines of Pascal output have at most this many characters
constexpr size_t stack_size   = 50;     /// number of simultaneous levels of macro expansion
constexpr size_t max_id_length
    = 12;  /// long identifiers are chopped to this length, which must not exceed line length
constexpr size_t unambig_length = 7;  /// identifiers must be unique if chopped to this length

// additional declarations to avoid magic constants
constexpr size_t max_modules = 027777;  /// 0x3FFF

// the maximum number of digits an int can have (in tangle)
constexpr size_t max_digits = 11;

}