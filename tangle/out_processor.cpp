#include "out_processor.h"

#include <cstdlib>

void
out_processor::commit_accumulator ()
{
    if (accumulator < 0 || (accumulator == 0 && pending_sign < 0))
    {
        out_buf.append (u8'-');
    }
    else if (next_sign > 0)
    {
        out_buf.append (next_sign);
    }
    out_buf.append_value (std::abs (accumulator));
    out_buf.flush_line_if_too_long ();
}

void
out_processor::commit_accumulator_if (bool needs_to_commit)
{
    if (needs_to_commit)
    {
        commit_accumulator ();
        next_sign   = u8'+';
        accumulator = pending_addend;
    }
    else
    {
        accumulator += pending_addend;
    }
}

void
out_processor::commit_sign ()
{
    out_buf.append (u8',' - pending_sign);
    out_buf.flush_line_if_too_long ();
    out_buf.mark_break ();
}

void
out_processor::commit_accumulator_if_necessary (bool needs_commit_when_sign_val_sign_val)
{
    switch (state)
    {
    case sign_val_sign:
        commit_accumulator ();
        commit_sign ();
        state = sign;
        break;

    case sign: 
        commit_sign ();
        break;

    case sign_val_sign_val:
        commit_accumulator_if (needs_commit_when_sign_val_sign_val); 
        commit_accumulator ();
        state = num_or_id;
        break;

    case sign_val:
        commit_accumulator ();
        state = num_or_id;
        break;
    }
}

void
out_processor::process_sign (int new_sign)
{
    switch (state)
    {
    case sign:
    case sign_val_sign: 
        pending_sign *= new_sign; 
        break;

    case sign_val:
        pending_sign = new_sign;
        state = sign_val_sign;
        break;

    case sign_val_sign_val:
        accumulator += pending_addend;
        pending_sign = new_sign;
        state = sign_val_sign;
        break;

    default:
        out_buf.mark_break ();
        pending_sign = new_sign;
        state = sign;
        break;
    }
}

void
out_processor::commit_decimal (int value)
{
    if (value >= 0)
    {
        if (state == num_or_id)
        {
            out_buf.mark_break ();
            out_buf.append (u8' ');
        }
        out_buf.append_value (value);
        out_buf.flush_line_if_too_long ();
        state = num_or_id;
    }
    else
    {
        out_buf.append (u8'(');
        out_buf.append (u8'-');
        out_buf.append_value (-value);
        out_buf.append (u8')');
        out_buf.flush_line_if_too_long ();
        state = misc;
    }
}

bool
previous_output_was_mult_or_div (const out_buffer &out_buf)
{
    auto s = out_buf.temporary_view_after_break ();
    return s == u8"*" || s == u8"/";
}

bool
previous_output_was_div_or_mod (const out_buffer &out_buf)
{
    auto s = out_buf.temporary_view_after_break ();
    return s == u8"DIV" || s == u8"MOD" || s == u8" DIV" || s == u8" MOD";
}

void
out_processor::process_value (int value)
{
    switch (state)
    {
    case num_or_id:
        if (previous_output_was_div_or_mod (out_buf))
        {
            commit_decimal (value);
            break;
        }

        next_sign   = u8' ';
        state       = sign_val;
        accumulator = value;
        out_buf.mark_break ();
        pending_sign = 1;
        return;

    case misc:
        if (previous_output_was_mult_or_div (out_buf))
        {
            commit_decimal (value);
            break;
        }

        next_sign   = 0;
        state       = sign_val;
        accumulator = value;
        out_buf.mark_break ();
        pending_sign = 1;
        return;

    case sign:
        next_sign   = u8'+';
        state       = sign_val;
        accumulator = pending_sign * value;
        return;

    case sign_val:
        state = sign_val_sign_val;
        pending_addend   = value;
        if (on_missing_sign_between_numbers)
        {
            on_missing_sign_between_numbers ();
        }
        return;

    case sign_val_sign:
        state = sign_val_sign_val;
        pending_addend = pending_sign * value;
        return;

    case sign_val_sign_val:
        accumulator += pending_addend;
        pending_addend = value;
        if (on_missing_sign_between_numbers)
        {
            on_missing_sign_between_numbers ();
        }
        return;

    default: 
        commit_decimal (value);
        break;
    }
}

static void 
append_all (out_buffer &out_buf, std::u8string_view content)
{
    for (auto c : content) { out_buf.append (c); }
    out_buf.flush_line_if_too_long ();
}

void
out_processor::process_fraction (std::u8string_view content)
{
    commit_accumulator_if_necessary (true);
    append_all (out_buf, content);
    state = num_or_id;
}

void
out_processor::process_identifier (std::u8string_view content)
{
    commit_accumulator_if_necessary (content == u8"DIV" || content == u8"MOD");
    if (state == misc)
    {
        out_buf.mark_break ();
    } 
    else if (state == num_or_id)
    {
        out_buf.mark_break ();
        out_buf.append (u8' ');
    }

    append_all (out_buf, content);
    state = num_or_id;
}

void
out_processor::process_string (std::u8string_view content)
{
    commit_accumulator_if_necessary (false);
    if (state == misc || state == num_or_id)
    {
        out_buf.mark_break ();
    }
    append_all (out_buf, content);
    state = misc;
}

void
out_processor::process_single_char (ascii_code_t c)
{
    commit_accumulator_if_necessary (c == u8'*' || c == u8'/');

    if (state == misc || state == num_or_id)
    {
        out_buf.mark_break ();
    }

    out_buf.append (c);
    out_buf.flush_line_if_too_long ();

    if (c == u8';' || c == u8'}')
    {
        out_buf.mark_break ();
        out_buf.mark_semicolon ();
    }

    state = misc;
}
