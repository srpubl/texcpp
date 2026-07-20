#pragma once

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace pascal
{

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
inline const std::string native_newline = "\r\n";  // Windows CRLF (13, 10)
#else
inline const std::string native_newline = "\n";  // Unix/Linux/macOS LF (10)
#endif

class text_file
{
  private:
    std::filesystem::path filepath;
    std::FILE            *file          = nullptr;
    char                  current_char  = ' ';
    bool                  is_eof        = true;
    bool                  is_eol        = false;
    bool                  is_input_mode = true;

    void
    read_next_char ()
    {
        if (!file)
            return;

        int next = std::fgetc (file);
        if (next == EOF)
        {
            is_eof       = true;
            is_eol       = false;
            current_char = ' ';  // Pascal defaults to space on EOF
        }
        else if (next == '\r')  // Handle Old Mac and Windows line endings
        {
            // Handle Windows \r\n line endings safely
            int peek_next = std::fgetc (file);
            if (peek_next != '\n' && peek_next != EOF)
            {
                std::ungetc (peek_next, file);
            }
            is_eol       = true;
            current_char = ' ';  // Pascal replaces newlines in the buffer with a space
        }
        else if (next == '\n')  // Handle Unix \n line endings
        {
            is_eol       = true;
            current_char = ' ';
        }
        else
        {
            is_eof       = false;
            is_eol       = false;
            current_char = static_cast<char> (next);
        }
    }

    inline void
    open (bool input_mode, const char *mode)
    {
        if (filepath.empty ())
            throw std::logic_error ("assign() not called before opening file");

        if (file)
        {
            std::fclose (file);
            file = nullptr;
        }

        is_input_mode = input_mode;

#if defined(_WIN32) && defined(_MSC_VER)
        // Safe, native wide-character path opening on Windows
        _wfopen_s (&file, filepath.c_str (), std::filesystem::path (mode).c_str ());
#else
        file = std::fopen (filepath.c_str (), mode);
#endif

        if (!file)
            // TODO: System could be out of memory, so the string concatenation would fail.
            throw std::runtime_error ("Could not open file: " + filepath.string ());

        is_eof = false;
        is_eol = false;
    }

    inline void
    check_is_input_mode ()
    {
        if (!is_input_mode)
            throw std::logic_error ("File is not opened in input mode");
    }

    inline void
    check_is_output_mode ()
    {
        if (is_input_mode)
            throw std::logic_error ("File is not opened in input mode");
    }

  public:
    text_file () {}

    ~text_file ()
    {
        if (file)
        {
            std::fclose (file);
        }
    }

    // Replaces Pascal: assign(f, path);
    void
    assign (const std::filesystem::path &path)
    { filepath = path; }

    // Replaces Pascal: reset(f);
    void
    reset ()
    {
        open (true, "rb");
        read_next_char ();
    }

    // Replaces Pascal: rewrite(f);
    void
    rewrite ()
    {
        open (false, "wb");
        current_char = ' ';
    }

    // Replaces Pascal: eof(f)
    bool
    eof () const
    { return is_eof; }

    // Replaces Pascal: eoln(f)
    bool
    eol () const
    { return is_eol; }

    // Replaces Pascal: f^
    char
    current () const
    { return current_char; }

    // Replaces Pascal: get(f)
    void
    get ()
    {
        check_is_input_mode ();
        read_next_char ();
    }

    // Replaces Pascal: read(f, ch)
    void
    read (char &ch)
    {
        check_is_input_mode ();

        if (is_eof)
        {
            ch = ' ';
        }
        else
        {
            ch = current_char;
            read_next_char ();
        }
    }

    // Replaces Pascal: readln(f)
    void
    read_line ()
    {
        check_is_input_mode ();

        while (!is_eol && !is_eof) { read_next_char (); }
        if (is_eol)
        {
            read_next_char ();
        }
    }

    // Replaces Pascal: write(f, ch)
    void
    write (char ch)
    {
        check_is_output_mode ();
        std::fputc (ch, file);
    }

    // Replaces Pascal: writeln(f)
    void
    write_line ()
    {
        check_is_output_mode ();
        std::fputs (native_newline.c_str (), file);
    }

    void
    close ()
    {
        if (file)
        {
            std::fclose (file);
            file = nullptr;
        }
    }
};

}  // namespace pascal
