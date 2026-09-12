#pragma once

// section 72

/// insertion of parameter
constexpr auto param         = char8_t {0x00};
constexpr auto verbatim      = char8_t {0x02};     /// @= begins a verbatim Pascal string, @> ends it
constexpr auto force_line    = char8_t {0x03};     /// @\ forces a new line in the Pascal output
constexpr auto begin_comment = char8_t {0x09};   /// @{ turns into { or [. in output
constexpr auto end_comment   = char8_t {0x0A};   /// @} turns into } or .] in output
constexpr auto octal         = char8_t {0x0C};   /// @' precedes an octal constant
constexpr auto hex           = char8_t {0x0D};   /// @" preceds a hex constant
constexpr auto double_dot    = char8_t {0x20};   /// denotes .. in Pascal
constexpr auto check_sum     = char8_t {0x7D};  /// @$ denotes the string pool check sum
constexpr auto join          = char8_t {0x7F};  /// @& is the item concatenation operator

