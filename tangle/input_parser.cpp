#include "input_parser.h"
#include "tokens.h"

auto
input_parser::handle_improper_numeric () -> bool 
{
    diagnose.on_improper_numeric_definition (in_tok_str.err ());

    do { current_token = in_tok_str.get_next_control_code (); }
    while (current_token < format);

    accumulator = 0; 
    return false;
}

void 
input_parser::parse_files (std::filesystem::path web_file_name, std::filesystem::path change_file_name) 
{
    in_tok_str.initialize (web_file_name, change_file_name);

    while (in_tok_str.get_next_control_code () != new_module) {}

    while (!in_tok_str.eof ()) { scan_module (); }

    in_tok_str.finalize ();        
}

auto
input_parser::process_numeric_definition_part () -> bool
{
    int val = 0;

    if (is_digit (current_token))
    {
        do
        {
            add_current_token (10, val);
        }
        while (is_digit (get_next_token ()));

        return update_accumulator (val);
    }

    switch (current_token)
    {
    case octal:
        current_token = u8'0';
        do
        {
            add_current_token (8, val);
        }
        while (is_octal (get_next_token ()));

        return update_accumulator (val);

    case hex:
        current_token = u8'0';
        do
        {
            if (current_token >= 'A')
            {
                current_token += u8'0' - (u8'A' - 10);
            }
            add_current_token (16, val);
        }
        while (is_hex (get_next_token ()));

        return update_accumulator (val);

    case identifier:
    {
        auto &name = in_tok_str.current_identifier (normal);
        if (name.ilk () != numeric)
            return handle_improper_numeric ();

        get_next_token ();
        return update_accumulator (name.number ());
    }

    case u8'+'       : 
        get_next_token ();
        return true;

    case u8'-'       : 
        current_sign = -current_sign; 
        get_next_token ();
        return true;

    case format      :
    case definition  :
    case module_name :
    case begin_pascal:
    case new_module  : 
        return false;

    case u8';':
        diagnose.on_semicolon_in_numeric_definition(in_tok_str.err ());
        get_next_token ();
        return true;

    default:
        return handle_improper_numeric ();
    }
}

/// defines numeric macros
auto input_parser::process_numeric_definition () -> int32_t
{
    accumulator  = 0;  
    current_sign = 1; 

    get_next_token ();
    while (process_numeric_definition_part ()) {}

    if (std::abs (accumulator) >= 0x8000)
    {
        diagnose.on_value_too_big_in_numeric_definition (in_tok_str.err (), accumulator);
        accumulator = 0;
    }
    return accumulator;
}

void
input_parser::ensure_parentheses_balance (text_manager::builder &builder, int &balance)
{
    if (balance > 0)
    {
        diagnose.on_unbalanced_parentheses (in_tok_str.err (), balance);
    }

    while (balance--)
    {
        builder << U')';
    }
}

void
input_parser::define_macro (ilk_value type)
{
    auto &name = in_tok_str.current_identifier (type);
    auto builder = text_mgr.make_builder ();
    scan_replacement (type, builder);
    auto &replacement_text = builder.finalize ();
    name.set_replacement_text (replacement_text);
    replacement_text.set_continuation (&text_mgr.root());
}

inline void
input_parser::scan_module ()
{
    scan_definition_part ();
    scan_pascal_part ();
}

void
input_parser::scan_definition_part ()
{
    current_token = 0;

    while (true)
    {
        while (current_token <= format)
        {
            current_token = in_tok_str.get_next_control_code ();;
        }

        if (current_token != definition)
            return;

        if (get_next_token () != identifier)
        {
            diagnose.on_no_valid_identifier_for_definition (in_tok_str.err ());
            continue;
        }
 
        switch (get_next_token ())
        {
        case u8'=':
            in_tok_str.current_identifier (numeric).set_number (process_numeric_definition());
            continue;

        case equivalence_sign:
            define_macro (simple);
            continue;

        case u8'(':
            if (get_next_token () == u8'#' && get_next_token () == u8')')
            {
                switch (get_next_token ())
                {
                case u8'=':
                    diagnose.on_missing_second_equal_sign_in_macro_definition (in_tok_str.err ());
                    [[fallthrough]];
                case equivalence_sign:
                    define_macro (parametric);
                    continue;
                }
            }
        }
    }
}

void
input_parser::scan_pascal_part ()
{
    name_t::optional_reference scanned_module_name = std::nullopt;
    switch (current_token)
    {
    case begin_pascal: break;
    case module_name:
        scanned_module_name = in_tok_str.current_module_name ();

        while (get_next_token () == u8'+') {}

        if (current_token == u8'=' || current_token != equivalence_sign)
            break;

        diagnose.on_missing_equal_sign_starting_pascal (in_tok_str.err ());

        while (in_tok_str.get_next_control_code () != new_module) {}
        return;

    default: 
        return;
    }

    auto builder = text_mgr.make_builder ();
    builder << (0xD000 + in_tok_str.current_module_count ());
    scan_replacement (module_name, builder);
    auto &replacement_text = builder.finalize ();
    replacement_text.set_continuation (nullptr);  // mark this replacement text as nonmacro

    if (!scanned_module_name.has_value())
    {
        text_mgr.add_unnamed (replacement_text);
    }
    else 
    {
        scanned_module_name -> get ().add_replacement_text (replacement_text);
    }
}


void
input_parser::scan_replacement (uint8_t type, text_manager::builder &builder)
{
    int     balance = 0;  /// left parentheses minus right parentheses
    bool    done = false;

    do
    {
        auto ch = get_next_token ();
        switch (ch)
        {
        case u8'(': 
            ++balance; 
            builder << static_cast <text_manager::char_type> (ch);
            break;

        case u8')':
            if (balance == 0)
            {
                diagnose.on_extra_parenthesis (in_tok_str.err ());
            }
            else
            {
                --balance;
            }
            builder << static_cast <text_manager::char_type> (ch);
            break;

        case u8'\'': 
            in_tok_str.copy_string_to (builder); 
            break;

        case u8'#':
            builder << static_cast <text_manager::char_type> (type == parametric ? param: ch);
            break;

        case identifier:
        {
            builder << 0x8000 + in_tok_str.current_identifier_id (normal);
            break;
        }

        case module_name:
            if (type == module_name)
            {
                builder << 0xA800 + in_tok_str.current_module_id ();
                break;
            }
            done = true;
            break;

        case verbatim: 
            in_tok_str.copy_verbatim_to (builder); 
            break;

        case definition:
        case format:
        case begin_pascal:       
            if (type == module_name)
            {
                diagnose.on_extra_control_code_in_replacement(in_tok_str.err (), ch);
                continue;
            }
            done = true;
            break;

        case new_module: done = true; break;
        
        default:
            builder  << static_cast <text_manager::char_type> (ch);
            break;
        }
    }
    while (!done);

    ensure_parentheses_balance (builder, balance);
}

