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

### send_out

First we split send_out in two functions: send_out_misc for the case that type == misc and send_out for all 
other cases to simplify the logic inside. send_out_misc doesn't need the type parameter anymore and the
parameter v actually indicates the character to send out.

Then in send_out, we introduce a local variable std::u8string_view content = {&out_contrib[1_r], v} and adapt
the copy loop accordingly such that there are no further mentions of out_contrib and v inside send_out. C&T.
We then replace the parameter v with content and adapt all calls to send_out(type, v) to 
send_out (type, {&out_contrib[1_r], v}). C&T. Now, we simplify all calls to send_out step by step, removing all 
references to out_contrib. send_out(frac, 0) -> send_out(frac, {&out_contrib[1_r], 0}) -> send_out(frac, 0).
send_out_string simplifies to a simple forwarding to send_out, so we remove it completely.

&out_contrib[1_r] = cur_char;
sent_out(ident, 1);
-> 
&out_contrib[1_r] = cur_char;
send_out (ident, {&out_contrib [1_r], 1}) 
-> 
send_out (ident, {&cur_char, 1});

For complex cases, we create a std::array<char8_t, n> buffer on the stack (n context-dependent). We need to take
care that out_contrib starts with index 1 but buffer with 0. Luckily, we are in a position to C&T after every 
transformation independently of the other transformations. It helps to first make the buffer one element larger, 
write send_out (str, {&buffer [1], n}), and in a second step adapt the indices and write 
send_out (str, {buffer.data(), n}). 

### Bundling all references to out_buf in its own class out_buffer

We create the class first with all members public. Out_buf becomes the first member called buffer. We keep the 
original out_buf but make it of type out_buffer. We then replace all occurences of out_buf by out_buf.buffer. 
As renaming doesn't work because of the dot, we use a trick: we rename out_buf to out_buf_buffer and then use
text replacement on that name. C&T.

We then do the same for all other new members: break_index, semi_index, line. After every member we C&T. For line
need to be careful: Knuth reuses the line from the input system; we disentangle those two and change only the
references to line when it is the output line.

Then we move small functions to buffer: app (renamed to append), app_val (append_value). flush_buffer (flush_line) can
now follow: everything referenced inside there is already in out_buffer. force_line_break will not be moved in its
entirety because it contains references to send_out and out_state, both of which will not be part of out_buffer.
Similarly, we disentangle empty_last_line_from_buffer (flush_last_line). 

We now want to make members private. Again we proceed stepwise. Realizing that most calls to buffer are actually:

out_buf.break_index = out_buf.buffer.size ();

We make that a function mark_break along with its companion mark_semicolon. The other use case is to check what's
inside the buffer, either all or after the break. So we added temporary_view and temporary_view_after_break.