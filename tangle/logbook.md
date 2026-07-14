# Log book

Started with an initial version that was more or less a 1:1 C++ translation of the original Pascal code
in tangle.web without too many C++'ish things.

Implemented pascal::range, which helped a lot identifying bugs as quickly locations in byte_mem and tok_mem
get overwritten with wrong values. Then the range limit causes exceptions that pinpoint bugs.

## Refactoring

### Moving input/output system and error handling out of tangle.cpp

Moved all print related functions to terminal.h, also text_char. ascii_code_t moved to character.h 
(with the idea that this could be upgraded to unicode support later).
Conversion between these two worlds happens in char_conversion.h. Naming based on the fact that text_char
is what is used for input and output, while all the logic runs on ascii_code_t.

Class terminal takes care of printing now, with the out stream a member. This will allow us later to run
several instances of tangle to run in the same process in parallel if needed. Class error_manager received
all the error functions. Note that err has a reference to a terminal and we took care to keep prints
separate (even though it is the same terminal right now): error messages are either passed to error.err_print
or printed via err.terminal().print*.

In a later stage we will make the respective err and term not global anymore but we
don't want to make them arguments to all possible functions now because there is hope that they will be
members of the class output_system and input_system.
