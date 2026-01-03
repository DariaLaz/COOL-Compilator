#include "CoolCodegen.h"

using namespace std;
#include "codegen/CodeEmitter.h"
#include <cmath>


void CoolCodegen::generate(ostream &out) {

    // Emit code here.

    // 1. create an "object model class table" that uses the class_table_ to compute the layout of objects in memory

    // 2. create a class to contain static constants (to be emitted at the end)

    // 3. emit code for method bodies; possibly append to static constants

    // 4. emit prototype objects

    // 5. emit dispatch tables

    // 6. emit initialization methods for classes

    // 7. emit class name table

    emit_name_table(out);

    // 8. emit static constants

    // Extra tip: implement code generation for expressions in a separate class and reuse it for method impls and init methods.


    // The following is an example manually written assembly that prints out "hello, world".

    // Start your work by removing it.

    auto names = class_table_->get_class_names();
}


void CoolCodegen::emit_name_table(ostream &out) {
    riscv_emit::emit_comment(out, "Class Name Table");
    riscv_emit::emit_p2align(out, 2);
    riscv_emit::emit_directive(out, "globl");
    out << " class_nameTab" << endl;
    riscv_emit::emit_label(out, "class_nameTab");

    vector<string> class_names = class_table_->get_class_names();
    vector<string> base_class_names = {"Object", "IO", "Int", "Bool", "String"};
    vector<string> other_class_names;

    for (const auto &name : class_names) {
        if (std::find(base_class_names.begin(), base_class_names.end(), name) == base_class_names.end()) {
            other_class_names.push_back(name);
        }
    }

    vector<string> all_class_names;
    all_class_names.insert(all_class_names.end(), base_class_names.begin(), base_class_names.end());
    all_class_names.insert(all_class_names.end(), other_class_names.begin(), other_class_names.end());

    for (const auto &class_name : all_class_names) {
        riscv_emit::emit_word(out, class_name + "_className");
    }

    riscv_emit::emit_empty_line(out);

    for (const auto &class_name : all_class_names) {
        emit_className_attributes(out, class_name);
    }
}

void CoolCodegen::emit_className_attributes(std::ostream &out, const std::string &class_name) {
    emit_length_attribute(out, class_name);
    emit_className(out, class_name);
}

void CoolCodegen::emit_length_attribute(std::ostream &out, const std::string &class_name) {
    riscv_emit::emit_gc_tag(out);
    riscv_emit::emit_label(out, class_name + "_classNameLength");
    riscv_emit::emit_word(out, 2);
    riscv_emit::emit_word(out, 4);
    riscv_emit::emit_word(out, 0);
    riscv_emit::emit_word(out,  class_name.length());
    riscv_emit::emit_empty_line(out);
}

void CoolCodegen::emit_className(std::ostream &out, const std::string &class_name) {
    riscv_emit::emit_gc_tag(out);
    riscv_emit::emit_label(out, class_name + "_className");
    riscv_emit::emit_word(out, 4);

    size_t str_len = class_name.length() + 1;
    size_t obj_size = ceil(str_len / 4.0) + 4;
    riscv_emit::emit_word(out, obj_size);
    riscv_emit::emit_word(out, "String_dispTab");
    riscv_emit::emit_word(out, class_name + "_classNameLength");
    riscv_emit::emit_word(out, "\"" + class_name + "\"");

    for (size_t i = str_len; i % 4 != 0; ++i) {
        riscv_emit::emit_byte(out, 0);
    }

    riscv_emit::emit_empty_line(out);
}