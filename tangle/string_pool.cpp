#include "string_pool.h"
#include "config.h"

void
string_pool::initialize (std::filesystem::path const &pool_file_name)
{
    _check_sum = 271828;
    _string_ptr = 256;
    pool.assign (pool_file_name);
    pool.rewrite ();
}


auto
string_pool::add (std::u8string_view str, size_t actual_length) -> config::index_t
{
    if (actual_length > 99)
    {
        diagnose.on_string_too_long ();
    }

    // output length
    write (char8_t (u8'0' + actual_length / 10));
    write (char8_t (u8'0' + actual_length % 10));

    add_to_checksum (actual_length);

    bool skip_one = true;  // skip first element and every doubled " or @
    for (auto ch : str)
    {
        if (skip_one)
        {
            skip_one = false;
            continue;
        }
        write (ch);
        add_to_checksum (ch);
        if (ch == u8'"' || ch == u8'@')
        {
            skip_one = true;
        }
    }
    pool.write_line ();

    return _string_ptr++;
}

void 
string_pool::finalize ()
{
    if (_string_ptr > 256)
    {
        diagnose.on_summary(_string_ptr - 256);
        write ('*');
        char digit_buffer [config::max_digits];
        std::to_chars (digit_buffer, std::end (digit_buffer), _check_sum);
        for (size_t i = 0; i < 9; ++i) { write (digit_buffer [i]); }
        pool.write_line ();
    }

    pool.close ();
}

void
string_pool::add_to_checksum (int value)
{
    _check_sum += _check_sum + value;
    while (_check_sum > checksum_prime) { _check_sum -= checksum_prime; }
}

