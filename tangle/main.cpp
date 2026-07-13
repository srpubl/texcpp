#include <print>

#include "tangle.h"

int
main (int argc, char **argv)
{
    try
    {
        if (argc != 5)
        {
            std::println ("Usage: {} web_file change_file Pascal_file pool_file", argv [0]);
            std::exit (1);
        }

        return tangle_exceptions_handled (argv [1], argv [2], argv [3], argv [4]);
    }
    catch (const std::exception &ex)
    {
        std::println (stderr, "\n[CRITICAL ERROR] Standard Exception Caught!");
        std::println (stderr, "What: {}", ex.what ());
        return 3;
    }
    catch (...)
    {
        std::println ("\n[CRITICAL ERROR] An unknown non-standard error occurred!");
        return 4;
    }
}
