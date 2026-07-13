#pragma once
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

struct file_diff_result_t
{
    bool   identical     = true;
    size_t byte_position = 0;
    size_t line_number   = 1;
    size_t char_on_line  = 0;
    char   expected_char = 0;
    char   actual_char   = 0;
};

auto
compare_files (const std::filesystem::path &expected_path, const std::filesystem::path &actual_path)
    -> file_diff_result_t
{
    auto result = file_diff_result_t {};
    auto f1     = std::ifstream {expected_path, std::ios::binary};
    auto f2     = std::ifstream {actual_path, std::ios::binary};

    if (!f1.is_open () || !f2.is_open ())
    {
        result.identical = false;
        return result;
    }

    char c1, c2;
    while (f1.get (c1))
    {
        result.byte_position++;
        result.char_on_line++;

        if (!f2.get (c2))
        {
            // Actual file is shorter than expected file
            result.identical     = false;
            result.expected_char = static_cast<unsigned char> (c1);
            return result;
        }

        if (c1 != c2)
        {
            result.identical     = false;
            result.expected_char = static_cast<unsigned char> (c1);
            result.actual_char   = static_cast<unsigned char> (c2);
            return result;
        }

        if (c1 == '\n')
        {
            result.line_number++;
            result.char_on_line = 0;
        }
    }

    if (f2.get (c2))
    {
        // Actual file is longer than expected file
        result.identical   = false;
        result.actual_char = static_cast<unsigned char> (c2);
    }

    return result;
}
