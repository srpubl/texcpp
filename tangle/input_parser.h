#pragma once

#include "error.h"
#include "input_token_stream.h"
#include "text_manager.h"

class input_parser 
{
public:
    struct diagnostics : public virtual input_token_stream::diagnostics
    {
        virtual void on_improper_numeric_definition (error_manager &err) = 0;
        virtual void on_semicolon_in_numeric_definition (error_manager &err) = 0;
        virtual void on_value_too_big_in_numeric_definition (error_manager &err, int value) = 0;
        virtual void on_unbalanced_parentheses (error_manager &err, int balance) = 0;
        virtual void on_extra_parenthesis (error_manager &err) = 0;
        virtual void on_missing_second_equal_sign_in_macro_definition (error_manager &err) = 0;
        virtual void on_no_valid_identifier_for_definition (error_manager &err) = 0;
        virtual void on_missing_equal_sign_starting_pascal (error_manager &err) = 0;
        virtual void on_extra_control_code_in_replacement (error_manager &err, char8_t code) = 0;
    };

private:
    diagnostics &
    diagnose;
    
    input_token_stream
    in_tok_str;

    text_manager &
    text_mgr;
    
    int accumulator;  /// accumulates sums
    int current_sign; /// sign to attach to next value

    char8_t current_token ;

public:
    input_parser (
        diagnostics &diagnose, 
        patched_in_stream &in_str, 
        name_manager &name_mgr, 
        text_manager &text_mgr) 
    : diagnose (diagnose)
    , in_tok_str (diagnose, in_str, name_mgr)
    , text_mgr (text_mgr)
    {}

    void parse_files (std::filesystem::path web_file_name, std::filesystem::path change_file_name);

private:
    char8_t
    get_next_token ()
    {
        return current_token = in_tok_str.get ();
    }

    auto
    update_accumulator (int value)
    {
        accumulator += current_sign * value;
        current_sign = 1;

        return true;
    }

    auto
    add_current_token (int base, int &value)
    {
        value = base * value + current_token - u8'0';
    }

    // returns true if it needs to be run again
    auto process_numeric_definition_part () -> bool;
    auto handle_improper_numeric () -> bool;
    auto process_numeric_definition () -> int32_t;
    void ensure_parentheses_balance (text_manager::builder &builder, int &balance);
    void scan_replacement (uint8_t type, text_manager::builder &builder);
    void define_macro (ilk_value type);
    void scan_definition_part ();
    void scan_pascal_part ();
    void scan_module ();
};

