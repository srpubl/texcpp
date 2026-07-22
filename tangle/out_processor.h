#pragma once

#include <cstdint>
#include <string_view>

#include "out_buffer.h"

using on_missing_sign_between_numbers_t       = void (*) ();

class out_processor
{
    enum state_enum
    {
        misc,
        num_or_id,          /// state associated with numbers and identifiers
        sign,               /// state associated with pending + or −
        sign_val,           /// state associated with pending sign and value
        sign_val_sign,      /// sign val followed by another pending sign
        sign_val_sign_val,  /// sign val followed by another pending value
        unbreakable         /// state associated with @&
    };

    out_buffer &out_buf;

    state_enum state    = misc; /// current status of partial output
    int accumulator     = 0;    /// pending value
    int pending_addend  = 0;    /// pending value
    int pending_sign    = 0;

    ascii_code_t next_sign = 0;

    on_missing_sign_between_numbers_t on_missing_sign_between_numbers = nullptr;

public:
    out_processor (out_buffer & out_buf)
        : out_buf (out_buf)
    {}

    void
    set_on_missing_sign_between_numbers (on_missing_sign_between_numbers_t f)
    { on_missing_sign_between_numbers = f; }

    void 
    ensure_no_line_break ()
    { state = unbreakable; }

    void
    force_line_break ()
    {
        process_string ({});  // normalize buffer
        out_buf.flush_all ();
        state = misc;
    }

    void
    process_sign (int new_sign);

    void
    process_value (int value);

    void
    process_fraction (std::u8string_view content);

    void
    process_identifier (std::u8string_view content);

    void
    process_string (std::u8string_view content);

    void
    process_single_char (ascii_code_t c);

private:
    void
    commit_accumulator_if_necessary (bool needs_commit_when_sign_val_sign);

    void
    commit_decimal (int value);

    void
    commit_accumulator ();

    void
    commit_sign ();
    
    void
    commit_accumulator_if (bool needs_to_commit);
};