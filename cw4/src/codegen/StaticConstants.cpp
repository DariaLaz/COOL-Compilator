#include "StaticConstants.h"

#include "codegen/CodeEmitter.h"
#include <cmath>

using namespace std;


string StaticConstants::use_string_constant(const string& str) {
    if (string_to_label.find(str) == string_to_label.end()) {
        string label = "str_const_" + to_string(next_string_id++);
        string_to_label[str] = label;
    }
    return string_to_label[str] + ".content";
}

string StaticConstants::use_bool_constant(bool value) {
    is_true_used = is_true_used || value;
    is_false_used = is_false_used || !value;
    return value ? "bool_const_true" : "bool_const_false";
}

string StaticConstants::use_int_constant(int value) {
    if (int_to_label.find(value) == int_to_label.end()) {
        string label = "int_const_" + to_string(value);
        int_to_label[value] = label;
    }
    return int_to_label[value];
}


void StaticConstants::emit_all(ostream &out) {
    riscv_emit::emit_header_comment(out, "Static Constants");

    for (const auto& [raw, label] : string_to_label) {
        string inner = strip_quotes_if_any(raw);
        string content = unescape_string_literal(inner);

        size_t str_len = content.size();

        riscv_emit::emit_gc_tag(out);
        riscv_emit::emit_label(out, label + ".length");
        riscv_emit::emit_word(out, 2);
        riscv_emit::emit_word(out, 4);
        riscv_emit::emit_word(out, 0);
        riscv_emit::emit_word(out, str_len);

        riscv_emit::emit_empty_line(out);

        riscv_emit::emit_gc_tag(out);
        riscv_emit::emit_label(out, label + ".content");
        riscv_emit::emit_word(out, 4);

        int str_len_with_null = str_len + 1;
        int obj_size = ceil(str_len_with_null / 4.0) + 4;

        riscv_emit::emit_word(out, obj_size);
        riscv_emit::emit_word(out, "String_dispTab");
        riscv_emit::emit_word(out, label + ".length");

        string asm_literal = "\"" + escape_for_gas_string(content) + "\"";
        riscv_emit::emit_string(out, asm_literal, "");

        for (size_t i = str_len_with_null; i % 4 != 0; ++i) {
            riscv_emit::emit_byte(out, 0);
        }

        riscv_emit::emit_empty_line(out);
    }

    if (is_true_used) {
        riscv_emit::emit_gc_tag(out);
        riscv_emit::emit_label(out, "bool_const_true");
        riscv_emit::emit_word(out, 3);
        riscv_emit::emit_word(out, 4);
        riscv_emit::emit_word(out, 0);
        riscv_emit::emit_word(out, 1);
        riscv_emit::emit_empty_line(out);
    }

    if (is_false_used) {
        riscv_emit::emit_gc_tag(out);
        riscv_emit::emit_label(out, "bool_const_false");
        riscv_emit::emit_word(out, 3);
        riscv_emit::emit_word(out, 4);
        riscv_emit::emit_word(out, 0);
        riscv_emit::emit_word(out, 0);
        riscv_emit::emit_empty_line(out);
    }

    for (const auto& [value, label] : int_to_label) {
        riscv_emit::emit_gc_tag(out);
        riscv_emit::emit_label(out, label);
        riscv_emit::emit_word(out, 2);
        riscv_emit::emit_word(out, 4);
        riscv_emit::emit_word(out, 0);
        riscv_emit::emit_word(out, value);
        riscv_emit::emit_empty_line(out);
    }



    out<<".globl _bool_tag\n\
_bool_tag:\n\
    .word 3\n\
\n\
.globl _int_tag\n\
_int_tag:\n\
    .word 2\n\
\n\
.globl _string_tag\n\
_string_tag:\n\
    .word 4\n";
}

string StaticConstants::unescape_string_literal(const string& raw) {
    string out;
    out.reserve(raw.size());

    for (size_t i = 0; i < raw.size(); ++i) {
        char c = raw[i];
        if (c != '\\') {
            out.push_back(c);
            continue;
        }
        
        if (i + 1 >= raw.size()) {
            out.push_back('\\');
            break;
        }

        char n = raw[++i];
        switch (n) {
            case 'n': out.push_back('\n'); break;
            case 't': out.push_back('\t'); break;
            case 'r': out.push_back('\r'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case '\\': out.push_back('\\'); break;
            case '"': out.push_back('"'); break;
            default:out.push_back(n); break;
        }
    }

    return out;
}


string StaticConstants::use_default_value(string class_name) {
    if (class_name == "Int") {
        return use_int_constant(0);
    } else if (class_name == "Bool") {
        return use_bool_constant(false);
    } else if (class_name == "String") {
        return use_string_constant("");
    } else {
        return "";
    }
}

string StaticConstants::strip_quotes_if_any(const string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

string StaticConstants::escape_for_gas_string(const string& s) {
    string out;
    out.reserve(s.size() + 8);

    for (char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\0': out += "\\0"; break;
            default:
                out += c;
                break;
        }
    }
    return out;
}