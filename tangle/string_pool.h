#pragma once

#include "pascal/text_file.h"
#include "config.h"
#include "character.h"


class string_pool 
{
public:
    struct diagnostics 
    {
        virtual void on_string_too_long () = 0;
        virtual void on_summary (config::index_t count) = 0;

        virtual ~diagnostics () = default;
    };

private:
    constexpr static int 
    checksum_prime = (1 << 29) - 73;

    diagnostics &diagnose;
    pascal::text_file pool;

    int _check_sum;
    config::index_t _string_ptr;

public:
    string_pool (diagnostics &diagnose) : diagnose (diagnose)
    {}

    auto &check_sum () const { return _check_sum; } 

    void initialize (std::filesystem::path const &pool_file_name);
    auto add (std::u8string_view str, size_t actual_length) -> config::index_t;
    void finalize ();

private:
    void add_to_checksum (int value);

    void
    write (char8_t c)
    { pool.write (convert_to_output (c)); }

    void
    write (char c)
    { pool.write (c); }

};

