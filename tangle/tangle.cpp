#include <climits>
#include <filesystem>
#include <print>

#include "config.h"

#include "error.h"
#include "input_parser.h"
#include "name_manager.h"
#include "out_buffer.h"
#include "out_processor.h"
#include "output_token_reducer.h"
#include "output_token_stream.h"
#include "patched_in_stream.h"
#include "string_pool.h"
#include "tangle.h"
#include "terminal.h"
#include "text_manager.h"

#include "diagnostics/change_stream_diagnostics.h"
#include "diagnostics/in_error_manager.h"
#include "diagnostics/in_stream_diagnostics.h"
#include "diagnostics/input_parser_diagnostics.h"
#include "diagnostics/name_manager_diagnostics.h"
#include "diagnostics/out_buffer_diagnostics.h"
#include "diagnostics/out_error_manager.h"
#include "diagnostics/out_processor_diagnostics.h"
#include "diagnostics/output_token_reducer_diagnostics.h"
#include "diagnostics/output_token_stream_diagnostics.h"
#include "diagnostics/patched_in_stream_diagnostics.h"
#include "diagnostics/string_pool_diagnostics.h"


static_assert (CHAR_BIT == 8, "Error: This codebase strictly requires an 8-bit char architecture");

terminal          term {stdout};
error_state       err_state {term};

out_error_manager out_err {err_state};

auto out_buf_diag = out_buffer_diagnostics {term, out_err};
auto out_buf = out_buffer {config::line_length, out_buf_diag};

text_manager text_mgr;

in_web_error_manager web_err {err_state};
in_web_error_manager change_err {err_state};

auto str_pool_diag = string_pool_diagnostics {term, web_err};
auto str_pool = string_pool {str_pool_diag};

auto name_mgr_diag = name_manager_diagnostics {web_err};
auto name_mgr     = name_manager {name_mgr_diag, str_pool};

auto output_token_str_diag = output_token_stream_diagnostics {out_err};
auto output_token_str      = output_token_stream {name_mgr, text_mgr, output_token_str_diag};
auto out_proc_diag         = out_processor_diagnostics {out_err};
auto out_proc              = out_processor {out_buf, out_proc_diag};
auto output_token_red_diag = output_token_reducer_diagnostics {out_err};
auto output_token_red      = output_token_reducer {
    output_token_str, out_proc, str_pool.check_sum(), output_token_red_diag};

void
output_compressed_tables (terminal &term)
{
    if (!text_mgr.root ().continuation ())
    {
        out_err.terminal ().print_nl ("! No output was specified.");
        out_err.mark_harmless ();
        return;
    }

    term.print_nl ("Writing the output file");
    term.update ();

    output_token_str.initialize();
    output_token_red.initialize ();
    output_token_red.send_the_output ();
    out_buf.flush_last_line ();

    auto brace_level = output_token_red.brace_level ();
    if (brace_level != 0)
    {
        out_err.err_print ("! Program ended at brace level {}", brace_level);
    }
    term.print_nl ("Done.");
}

patched_in_stream_diagnostics patched_in_diag {web_err, change_err};
in_stream_diagnostics web_str_diag {web_err};
change_stream_diagnostics change_str_diag {change_err};

patched_in_stream in_str {patched_in_diag, web_str_diag, change_str_diag, config::buf_size - 1};

input_parser_diagnostics parser_diag {term};
input_parser parser {parser_diag, in_str, name_mgr, text_mgr};

int
tangle (
    std::filesystem::path web_file_name,
    std::filesystem::path change_file_name,
    std::filesystem::path pascal_file_name,
    std::filesystem::path pool_file_name)
{
    str_pool.initialize (pool_file_name);
    name_mgr.initialize (config::max_bytes, config::max_names);
    text_mgr.initialize (config::max_toks, config::max_texts);

    term.print_ln ("{}", config::banner);

    parser.parse_files (web_file_name, change_file_name);

    out_buf.initialize (pascal_file_name);
    out_err.set_buffer (&out_buf);
    output_compressed_tables (term);
    str_pool.finalize ();
    out_buf.finalize ();

    return err_state.exit_code ();
}

int
tangle_exceptions_handled (
    std::filesystem::path web_file_name,
    std::filesystem::path change_file_name,
    std::filesystem::path pascal_file_name,
    std::filesystem::path pool_file_name)
{
    try
    {
        return tangle (web_file_name, change_file_name, pascal_file_name, pool_file_name);
    }
    catch (const std::filesystem::filesystem_error &ex)
    {
        std::println (stderr);
        std::println (stderr, "[CRITICAL ERROR] File system exception!");
        std::println (stderr, "What: {}", ex.what ());
        std::println (stderr, "Path 1: {}", ex.path1 ().string ());
        return 4;
    }
    catch (const std::exception &ex)
    {
        std::println (stderr);
        std::println (stderr, "[CRITICAL ERROR] Standard Exception Caught!");
        std::println (stderr, "What: {}", ex.what ());
        return 5;
    }
    catch (...)
    {
        std::println (stderr);
        std::println (stderr, "[CRITICAL ERROR] An unknown non-standard error occurred!");
        return 6;
    }
}

