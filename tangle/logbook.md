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

### out_processor

The variables out_state, out_val, out_app, out_sign, last_sign are highly coherent. We move them into a class
out_processor with similar steps as for out_buffer. Renaming of functions was crucial as the original names hid
their intents. We now have streamlined names: commit_x transfers x to the out_buffer for good; those are all
private. process_x form the public interface of the class, which receive x, change the internal state accordingly
and occassionally commits something to the out_buffer. 

We also rename out_val to accumulator to more closely follow processor naming. out_sign gets renamed next_sign (to 
avoid naming conflict with the state called sign). out_app has two completely separate purposes: it serves as 
the pending sign and the pending second value (there is an optimization that chains of additions/subtractions 
of literals get merged into a single lingle). That optimization makes the system extremely hard to understand.
We therefore split out_app into two distinct members pending_sign and pending_addend. It then turns out that 
last_sign and pending_sign can be the same variable.

Restructuring the original state machine in prepare_buffer_for_append (commit_accumulator_if_necessary) is a major
task: In the original tangle, it is a big switch with many gotos. We implemented it already as a while loop with 
a switch inside. Careful analysis reveals that in the worst case the while loop terminates rather quickly as
states progress only in one direction. Essentially, it is a complicated way of specifying a related class of 
instruction sequences. 

First, we make those sequences explicit per state. Then we realize that all the if's depend on the type, which is 
hard-coded for each call to send_out. We therefore make an explicit version per type: process_string, 
process_fraction, process_identifier and process_single_char (for misc). (process_sign and process_value were 
already separate functions.) 

Inside the state machine, we merge the states num_or_id and misc because they are nearly similar (adding an 
additional if (state == num_or_id) inside). We can then move this out of the switch at the end of the function
and in the next step move it from there after each call-site. There finally, we can replace type with the
actual type used and reduce it (e.g. in process_fraction it vanished completely). The state machine thus gets 
reduced to 4 states with simple code and 4 process_x functions with 3 to 12 lines of code, each.

### send_output_one_char

We want to get rid of the while loop. So we revamp everything into one big switch as in the original tangle as
this will make things easier. In order to avoid 62 labels for 0-9A-Za-z, we revert to macros to generate cases
(there is no other way in C++ except for relying on non-standard syntax). Most things are straightforward, like
moving cases out of functions back into the big switch. For constants, it's a bit more complicated as the
function returns three possibilities: consumed, reswitch, not_consumed. We want to get rid of the reswitch
because then the return value is a boolean (consumed vs not_consumed) as in the other functions.

The only reason reswitch exists is because sometimes we need to peek at the next output token without consuming 
it. Fortunately, we only need to look ahead one token. So we implement a simple put_back_output that just
stores one token. We adapt get_output by renaming it and writing a little wrapper get_output around it that
just checks whether there is an output token that was put back. We need to take care to add an additional
check to send_the_output as this loop needs to continue if there is still a character that was put back. We
also implement a little helper peek_output, which will come in handy later.

Now, we can replace all continue statements in our loop by put_back_output(cur_char); return. In 
send_output_constant, we replace return reswitch with put_back_output(cur_char); return consumed. Thus, we can 
move all cases from this function to our big switch. We now have no continue or break in that big switch, so
we can remove the outer while loop and replace all returns with breaks.

We then remove the cur_char parameter from finish_real_constant and instead put the char back before calling
the function and at its end thus reducing dependencies. We also realize now that send_out_number is always
followed by put_back_char, so we move that call into it, and pass cur_char by value instead of by reference.
And now we see that we very often can just use peek_char, making the code much easier to understand. With
a couple of cleanups we manage to get the switch nicely structured and really easy to comprehend.


### Identifiers / strings

The allocation of memory is somewhat awkward as the original tangle was constricted to a 16-bit architecture.
It therefore used a couple of arrays for allocating space for strings and identifiers. We want to replace
this with a modern approach but we want to keep the efficiency of the allocation as strings and identifiers
always get allocated consecutively and rarely deleted (and then only the item allocated last).

We start by replacing easy locate_byte_mem calls with the new function name (p), which avoids already dealing
with the memory layout in most places. We then look at byte_start: this array holds indices into byte_mem.
A name is identified by its index into byte_start the content of which is the first byte of the name in 
byte_mem. The original tangle uses ww byte_mem arrays but only one byte_start array. The names get assigned
to arrays round robin: name 0 is in byte_mem 0, name 1 in byte_mem 1, etc and name ww again in byte_mem 0.
There is the index name_ptr that signifies the first index not in use. Also, the first ww entries in
byte_start are 0 by definition. 

We thus define a vector name_start that we statically allocate (via reserve) with the required number of 
entries. Note the difference between capacity and size of a vector: capacity is the allocated storage place, 
size denotes how much of that is actually used. We change name_ptr into a function: auto name_ptr ()
{ return name_pointer_t {name_start.size () - ww};}. In this way, we don't have to keep track of name_ptr in
addition to the vector itself. Now, most places are easy to adjust to name_start and name_ptr. Where we add
content, we just replace the respective lines with name_start.push_back, and the one occasion of --name_ptr
gets replaced with name_start.pop_back. Similarly we replace byte_mem with a vector and remove byte_ptr. 

We can now tackle the memory limitations and their workarounds. First, we need to increase the size of an
index to 32 bits. To avoid overflows we consequently replace all uint16_t by index_t, which we define as an
alias to uint32_t. Then we set ww to 1 and increase max_bytes accordingly. We then flatten byte_mem into a 
single std::array and adjust all references to it accordingly, simplifying as much as possible.

We also rename name_pointer_t to index_t, and change ilk, equiv, and link into std::arrays.





