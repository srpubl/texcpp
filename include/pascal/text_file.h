#pragma once

#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <stdexcept>

namespace pascal 
{

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
inline const std::string native_newline = "\r\n"; // Windows CRLF (13, 10)
#else
inline const std::string native_newline = "\n"; // Unix/Linux/macOS LF (10)
#endif

class text_file 
{
private:
    std::fstream stream;
    std::filesystem::path filepath;

    char current_char = ' ';
    bool is_eof = true;
    bool is_eol = false;
    bool is_input_mode = true;

    void 
    read_next_char () 
    {
        if (!stream.is_open()) 
            return;

        int next = stream.get();
        if (next == EOF) 
        {
            is_eof = true;
            is_eol = false;
            current_char = ' '; // Pascal defaults to space on EOF
        } 
		else if (next == '\r') // Handle Old Mac and Windows line endings
        {
            // Handle Windows \r\n line endings safely
            int peek_next = stream.peek();
            if (peek_next == '\n') 
            {
                stream.get();
            }
            is_eol = true;
            current_char = ' '; // Pascal replaces newlines in the buffer with a space
        }
        else if (next == '\n') // Handle Unix \n line endings
        {
            is_eol = true;
            current_char = ' ';
        }
        else 
        {
            is_eof = false;
            is_eol = false;
            current_char = static_cast<char>(next);
        }
    }

    inline void
    open (bool input_mode, std::ios_base::openmode mode)
    {
        if (filepath.empty ())
            throw std::logic_error ("assign() not called before opening file");

        if (stream.is_open ())
        {
            stream.close ();
        }

        is_input_mode = input_mode;
        stream.open(filepath, mode);

        if (!stream)
            // TODO: System could be out of memory, so the string concatenation would fail.
            throw std::runtime_error("Could not open file: " + filepath.string());

        is_eof = false;
        is_eol = false;
    }

    inline void
	check_is_input_mode() 
    {
		if (!is_input_mode)
			throw std::logic_error("File is not opened in input mode");
	}

    inline void
    check_is_output_mode()
    {
        if (is_input_mode)
            throw std::logic_error("File is not opened in input mode");
    }



public:
    text_file () {}

    // Replaces Pascal: assign(f, path);
    void 
    assign(const std::filesystem::path &path) 
    {
        filepath = path;
    }

    // Replaces Pascal: reset(f);
    void 
    reset () 
    {
		open (true, std::ios::in | std::ios::binary);        
        read_next_char (); 
    }

    // Replaces Pascal: rewrite(f);
    void 
    rewrite () 
    {
        open(false, std::ios::out | std::ios::trunc | std::ios::binary);
        current_char = ' ';
    }

    // Replaces Pascal: eof(f)
    bool 
    eof () const { return is_eof; }

    // Replaces Pascal: eoln(f)
    bool 
    eol () const { return is_eol; }

	// Replaces Pascal: f^
    char
	current() const { return current_char; }

	// Replaces Pascal: get(f)
    void
	get() 
    {
		check_is_input_mode();
		read_next_char();
	}

    // Replaces Pascal: read(f, ch)
    void 
    read (char& ch) 
    {
		check_is_input_mode();
        
        if (is_eof) 
        {
            ch = ' ';
        } 
        else 
        {
            ch = current_char;
            read_next_char();
        }
    }

    // Replaces Pascal: readln(f)
    void 
    read_line () 
    {
        check_is_input_mode();

        while (!is_eol && !is_eof) 
        {
            read_next_char();
        }
        if (is_eol) 
        {
            read_next_char();
        }
    }

    // Replaces Pascal: write(f, ch) or write(f, string)
    template <typename T>
    void write (const T& val) 
    {
		check_is_output_mode();
        stream << val;
    }

    // Replaces Pascal: writeln(f)
    void 
    write_line () 
    {
		check_is_output_mode();
        stream << native_newline;
    }

    void 
    close ()
    {
        stream.close();
    }
};

}
