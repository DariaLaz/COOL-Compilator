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
    riscv_emit::emit_comment(out, "Static Constants");

    for (const auto& [str, label] : string_to_label) {
        riscv_emit::emit_gc_tag(out);
        riscv_emit::emit_label(out, label + ".length");
        riscv_emit::emit_word(out, 2);
        riscv_emit::emit_word(out, 4);
        riscv_emit::emit_word(out, 0);
        riscv_emit::emit_word(out, str.length());

        riscv_emit::emit_empty_line(out);

        riscv_emit::emit_gc_tag(out);
        riscv_emit::emit_label(out, label + ".content");
        riscv_emit::emit_word(out, 4);
        size_t str_len = str.length() + 1;
        size_t obj_size = ceil(str_len / 4.0) + 4;
        riscv_emit::emit_word(out, obj_size);
        riscv_emit::emit_word(out, "String_dispTab");
        riscv_emit::emit_word(out, label + ".length");
        riscv_emit::emit_word(out, "\"" + str + "\"");

        for (size_t i = str_len; i % 4 != 0; ++i) {
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
}
