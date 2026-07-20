#pragma once

#include <cstdio>
#include <print>
#include <utility>

class terminal
{
    std::FILE *stream = stdout;

public:
    terminal () : stream (stdout) {}
    explicit terminal (std::FILE *stream) : stream (stream) {}

    terminal (const terminal &)     = default;
    terminal (terminal &&) noexcept = default;
    terminal &
    operator= (const terminal &) = default;
    terminal &
    operator= (terminal &&) noexcept = default;

    ~terminal ()                     = default;

    /// Prints the format with arguments
    template <typename... Args>
        requires (sizeof...(Args) > 0)
    void
    print (std::format_string<Args...> fmt, Args &&...args)
    { std::print (stream, fmt, std::forward<Args> (args)...); }

    inline void
    print (std::string_view str)
    { std::print (stream, "{}", str); }

    // Special version because this case happens so often
    inline void
    print (char ch)
    { std::fputc (ch, stream); }

    template <typename... Args>
    inline void
    print_ln (std::format_string<Args...> fmt, Args &&...args)
    { std::println (stream, fmt, std::forward<Args> (args)...); }

    inline void
    newline ()
    { std::println (stream); }

    template <typename... Args>
    void
    print_nl (std::format_string<Args...> fmt, Args &&...args)
    {
        newline ();
        std::print (stream, fmt, std::forward<Args> (args)...);
    }

    inline void
    update ()
    { std::fflush (stream); }
};
