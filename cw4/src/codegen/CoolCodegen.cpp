#include "CoolCodegen.h"

using namespace std;
#include "codegen/CodeEmitter.h"
#include "codegen/Register.h"
#include <cmath>



void CoolCodegen::generate(ostream &out) {
    emit_methods(out);

    emit_tables(out);

    static_constants_.emit_all(out);
}


void CoolCodegen::emit_methods(ostream &out) {
    riscv_emit::emit_directive(out, "text");
    riscv_emit::emit_empty_line(out);

    riscv_emit::emit_label(out, "_inf_loop");
    out << "    j _inf_loop" << endl;
    riscv_emit::emit_empty_line(out);
    
    riscv_emit::emit_header_comment(out, "Method Implementations");

    int num_classes = class_table_->get_num_of_classes();

    vector<string> base_class_names = {"Object", "IO", "Int", "Bool", "String"};

    for (int class_index = 0; class_index < num_classes; ++class_index) {
        string_view class_name_sv = class_table_->get_name(class_index);
        string class_name(class_name_sv.data(), class_name_sv.size());

        if (find(base_class_names.begin(), base_class_names.end(), class_name) != base_class_names.end()) {
            continue;
        }

        auto methods = class_table_->get_method_names(class_index);

        for (const auto &method_name : methods) {
            riscv_emit::emit_empty_line(out);
            riscv_emit::emit_directive(out, "globl");
            string function_label = class_name + "." + method_name;
            out<< " " << function_label << endl;
            riscv_emit::emit_label(out, function_label);

            // ================================ PROLOGUE =================================
            // fp = sp
            riscv_emit::emit_add(out, FramePointer{}, StackPointer{}, ZeroRegister{});

            // save ra at 0(fp)
            riscv_emit::emit_store_word(out, ReturnAddress{}, MemoryLocation{0, StackPointer{}});
            riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);

            // save s1 (we keep self in s1) at -4(fp)
            riscv_emit::emit_store_word(out, SavedRegister{1}, MemoryLocation{0, StackPointer{}});
            riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);

            // s1 = self (a0)
            riscv_emit::emit_add(out, SavedRegister{1}, ArgumentRegister{0}, ZeroRegister{});
            riscv_emit::emit_empty_line(out);
            // ============================== END PROLOGUE ===============================

            expression_codegen_.reset_frame();
            expression_codegen_.set_current_class(class_index);
            expression_codegen_.begin_scope();
             auto formals = class_table_->get_argument_names(class_index, method_name);
            expression_codegen_.bind_formals(formals);

            expression_codegen_.generate(out, class_table_->get_method_body(class_index, method_name));
            
            expression_codegen_.end_scope();

            // ================================ EPILOGUE =================================
            // restore s1 (saved at -4(fp))
                riscv_emit::emit_load_word(out, SavedRegister{1}, MemoryLocation{-4, FramePointer{}});

                // restore ra (saved at 0(fp))
                riscv_emit::emit_load_word(out, ReturnAddress{}, MemoryLocation{0, FramePointer{}});

                // IMPORTANT: caller pushes control link FIRST, then args.
                // control link is at fp + 4*(argc+1).
                // We want sp to end up at the control link word (so we can lw fp, 0(sp)).
                //
                // Current sp at end of method is: fp - 4*(1 + saved_regs)
                // (we subtracted 4 for ra slot, and 4 per saved reg)
                //
                // Therefore delta = 4*(argc + saved_regs + 2)
                int saved_regs = 1;                       // we save only s1
                int argc = (int)formals.size();           // number of method formals
                int pop_bytes = 4 * (argc + saved_regs + 2);
                riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, pop_bytes);

                // restore fp from control link (now at 0(sp))
                riscv_emit::emit_load_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});

                riscv_emit::emit_mnemonic(out, Mnemonic::Return);
                riscv_emit::emit_empty_line(out);
            // ============================== END EPILOGUE ===============================
        }
    }

    riscv_emit::emit_empty_line(out);
}

void CoolCodegen::emit_tables(ostream &out) {
    riscv_emit::emit_directive(out, "data");
    riscv_emit::emit_empty_line(out);

    vector<string> class_names = class_table_->get_class_names();
    vector<string> base_class_names = {"Object", "IO", "Int", "Bool", "String"};
    vector<string> other_class_names;

    for (const auto &name : class_names) {
        if (find(base_class_names.begin(), base_class_names.end(), name) == base_class_names.end()) {
            other_class_names.push_back(name);
        }
    }

    vector<string> all_class_names;
    all_class_names.insert(all_class_names.end(), base_class_names.begin(), base_class_names.end());
    all_class_names.insert(all_class_names.end(), other_class_names.begin(), other_class_names.end());


    emit_name_table(out, all_class_names);
    emit_prototype_tables(out, all_class_names);
    emit_dispatch_tables(out, all_class_names, base_class_names);
    emit_initialization_methods(out, all_class_names);
    emit_class_object_table(out, all_class_names);
}

void CoolCodegen::emit_name_table(ostream &out, vector<string>& class_names) {
    riscv_emit::emit_header_comment(out, "Class Name Table");
    riscv_emit::emit_p2align(out, 2);
    riscv_emit::emit_directive(out, "globl");
    out << " class_nameTab" << endl;
    riscv_emit::emit_label(out, "class_nameTab");
    
    for (const auto &class_name : class_names) {
        riscv_emit::emit_word(out, class_name + "_className");
    }

    riscv_emit::emit_empty_line(out);

    for (const auto &class_name : class_names) {
        emit_className_attributes(out, class_name);
    }
}

void CoolCodegen::emit_className_attributes(ostream &out, const string &class_name) {
    emit_length_attribute(out, class_name);
    emit_className(out, class_name);
}

void CoolCodegen::emit_length_attribute(ostream &out, const string &class_name) {
    riscv_emit::emit_gc_tag(out);
    riscv_emit::emit_label(out, class_name + "_classNameLength");
    riscv_emit::emit_word(out, 2);
    riscv_emit::emit_word(out, 4);
    riscv_emit::emit_word(out, 0);
    riscv_emit::emit_word(out,  class_name.length());
    riscv_emit::emit_empty_line(out);
}

void CoolCodegen::emit_className(ostream &out, const string &class_name) {
    riscv_emit::emit_gc_tag(out);
    riscv_emit::emit_label(out, class_name + "_className");
    riscv_emit::emit_word(out, 4);

    size_t str_len = class_name.length() + 1;
    size_t obj_size = ceil(str_len / 4.0) + 4;
    riscv_emit::emit_word(out, obj_size);
    riscv_emit::emit_word(out, "String_dispTab");
    riscv_emit::emit_word(out, class_name + "_classNameLength");
    riscv_emit::emit_string(out, "\"" + class_name + "\"");

    for (size_t i = str_len; i % 4 != 0; ++i) {
        riscv_emit::emit_byte(out, 0);
    }

    riscv_emit::emit_empty_line(out);
}

void CoolCodegen::emit_prototype_tables(ostream &out, vector<string>& class_names) {
    riscv_emit::emit_header_comment(out, "Prototype Object Table");
    riscv_emit::emit_p2align(out, 2);
    
    for (size_t i = 0; i < class_names.size(); ++i) {
        emit_prototype_table(out, class_names[i], i);
    }
}

void CoolCodegen::emit_prototype_table(ostream &out, const string &class_name, size_t index) {
    riscv_emit::emit_gc_tag(out);
    riscv_emit::emit_directive(out, "globl");
    out << " " << class_name << "_protObj" << endl;
    riscv_emit::emit_label(out, class_name + "_protObj");
    riscv_emit::emit_word(out, class_table_->get_index(class_name));

    
    if (class_name == "Object") {
        riscv_emit::emit_word(out, 3);
        riscv_emit::emit_word(out, "Object_dispTab");
    } else if (class_name == "IO") {
        riscv_emit::emit_word(out, 3);
        riscv_emit::emit_word(out, "IO_dispTab");
    } else if (class_name == "Int") {
        riscv_emit::emit_word(out, 4);
        riscv_emit::emit_word(out, 0);
        riscv_emit::emit_word(out, 0);
    } else if (class_name == "Bool") {
        riscv_emit::emit_word(out, 4);
        riscv_emit::emit_word(out, 0);
        riscv_emit::emit_word(out, 0);
    } else if (class_name == "String") {
        riscv_emit::emit_word(out, 5);
        riscv_emit::emit_word(out, "String_dispTab");
        riscv_emit::emit_word(out, 0);
        riscv_emit::emit_word(out, 0);
    } else {
        size_t class_index = class_table_->get_index(class_name);
        auto attributes = class_table_->get_attributes(class_index);

        size_t obj_size = 3 + attributes.size();
        riscv_emit::emit_word(out, obj_size);
        riscv_emit::emit_word(out, class_name + "_dispTab");

        for (string &attr_name : attributes) {
            auto attr_type_index = class_table_->get_attribute_type(class_index, attr_name);
            if (!attr_type_index) {
                riscv_emit::emit_word(out, 0);
                continue;
            }

            string_view attr_type_sv = class_table_->get_name(attr_type_index.value());
            string attr_type(attr_type_sv.data(), attr_type_sv.size());

            if (attr_type == "Int") {
                riscv_emit::emit_word(out, static_constants_.use_int_constant(0));
            } else if (attr_type == "Bool") {
                riscv_emit::emit_word(out, static_constants_.use_bool_constant(false));
            } else if (attr_type == "String") {
                riscv_emit::emit_word(out, static_constants_.use_default_value("String"));
            } else {
                riscv_emit::emit_word(out, 0);
            }
        }
    }
    
    riscv_emit::emit_empty_line(out);
}


void CoolCodegen::emit_dispatch_tables(ostream &out, vector<string>& class_names, vector<string>& base_class_names) {
    riscv_emit::emit_header_comment(out, "Dispatch Tables");
    for (const auto &class_name : class_names) {
        emit_dispatch_table(out, class_name, base_class_names);
    }
}

void CoolCodegen::emit_dispatch_table(ostream &out, const string& class_name, vector<string>& base_class_names) {
    if (find(base_class_names.begin(), base_class_names.end(), class_name) != base_class_names.end()) {
        riscv_emit::emit_directive(out, "globl");
        out << " " << class_name << "_dispTab" << endl;
    }

    riscv_emit::emit_label(out, class_name + "_dispTab");
    size_t class_index = class_table_->get_index(class_name);
    auto methods = class_table_->get_all_methods(class_index);

    for (const auto &method_name : methods) {
        auto current_class_name_sv =
            class_table_->get_name(method_name.second);
        string current_class_name(current_class_name_sv.data(),
                                       current_class_name_sv.size());
        riscv_emit::emit_word(out, current_class_name + "." + method_name.first);
    }

    riscv_emit::emit_empty_line(out);
}

void CoolCodegen::emit_initialization_methods(ostream &out, vector<string>& class_names) {
    riscv_emit::emit_header_comment(out, "Initialization Methods");
    
    out << ".globl Object_init\n\
Object_init:\n\
    # Most of the `init` functions of the default types are no-ops, so the\n\
    # implementation is the same.\n\
\n\
    # stack discipline:\n\
    # callee:\n\
    # - activation frame starts at the stack pointer\n\
    add fp, sp, 0\n\
    # - previous return address is first on the activation frame\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
    # before using saved registers (s1 -- s11), push them on the stack\n\
\n\
    # no op\n\
\n\
    # stack discipline:\n\
    # callee:\n\
    # - restore used saved registers (s1 -- s11) from the stack\n\
    # - ra is restored from first word on activation frame\n\
    lw ra, 0(fp)\n\
    # - ra, arguments, and control link are popped from the stack\n\
    addi sp, sp, 8\n\
    # - fp is restored from control link\n\
    lw fp, 0(sp)\n\
    # - result is stored in a0\n\
\n\
    ret\n\
\n\
\n\
.globl IO_init\n\
IO_init:\n\
    # Most of the `init` functions of the default types are no-ops, so the\n\
    # implementation is the same.\n\
\n\
    add fp, sp, 0\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    # no op\n\
\n\
    lw ra, 0(fp)\n\
    addi sp, sp, 8\n\
    lw fp, 0(sp)\n\
    ret\n\
\n\
\n\
# Initializes an object of class Int passed in a0. In practice, a no-op, since\n\
# Int_protObj already has the first (and only) attribute set to 0.\n\
.globl Int_init\n\
Int_init:\n\
    # Most of the `init` functions of the default types are no-ops, so the\n\
    # implementation is the same.\n\
\n\
    add fp, sp, 0\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    # no op\n\
\n\
    lw ra, 0(fp)\n\
    addi sp, sp, 8\n\
    lw fp, 0(sp)\n\
    ret\n\
\n\
\n\
# Initializes an object of class Bool passed in a0. In practice, a no-op, since\n\
# Bool_protObj already has the first (and only) attribute set to 0.\n\
.globl Bool_init\n\
Bool_init:\n\
    # Most of the `init` functions of the default types are no-ops, so the\n\
    # implementation is the same.\n\
\n\
    add fp, sp, 0\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    # no op\n\
\n\
    lw ra, 0(fp)\n\
    addi sp, sp, 8\n\
    lw fp, 0(sp)\n\
    ret\n\
\n\
\n\
# Initializes an object of class String passed in a0. Allocates a new Int to\n\
# store the length of the String and links the length pointer to it. Returns the\n\
# initialized String in a0.\n\
#\n\
# Used in `new String`, but useless, in general, since it creates an empty\n\
# string. String only has methods `length`, `concat`, and `substr`.\n\
.globl String_init\n\
String_init:\n\
    # In addition to the default behavior, copies the Int prototype object and\n\
    # uses that as the length, rather than the prototype object directly. No\n\
    # practical reason for this, other than simulating the default init logic for\n\
    # an object with attributes.\n\
\n\
    add fp, sp, 0\n\
    sw ra, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    # store String argument\n\
    sw s1, 0(sp)\n\
    addi sp, sp, -4\n\
    add s1, a0, zero\n\
\n\
    # copy Int prototype first\n\
\n\
    la a0, Int_protObj\n\
    sw fp, 0(sp)\n\
    addi sp, sp, -4\n\
\n\
    call Object.copy\n\
\n\
    sw a0, 12(s1)      # store new Int as length; value of Int is 0 by default\n\
\n\
    add a0, s1, zero   # restore String argument\n\
\n\
    addi sp, sp, 4\n\
    lw s1, 0(sp)\n\
    lw ra, 0(fp)\n\
    addi sp, sp, 8\n\
    lw fp, 0(sp)\n\
\n\
    ret\n";

    vector<string> base = {"Object","IO","Int","Bool","String"};

    for (const auto& cls : class_names) {
        if (find(base.begin(), base.end(), cls) != base.end()) continue;

        riscv_emit::emit_globl(out, cls + "_init");
        riscv_emit::emit_label(out, cls + "_init");

        riscv_emit::emit_add(out, FramePointer{}, StackPointer{}, ZeroRegister{});
        riscv_emit::emit_store_word(out, ReturnAddress{}, MemoryLocation{0, StackPointer{}});
        riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);

        // save s1
        riscv_emit::emit_store_word(out, SavedRegister{1}, MemoryLocation{0, StackPointer{}});
        riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);

        // s1 = self
        riscv_emit::emit_add(out, SavedRegister{1}, ArgumentRegister{0}, ZeroRegister{});
        riscv_emit::emit_empty_line(out);

        int class_index = class_table_->get_index(cls);
        std::string parent(class_table_->get_name(class_table_->get_parent_index(class_index)));

        riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
        riscv_emit::emit_add(out, ArgumentRegister{0}, SavedRegister{1}, ZeroRegister{});
        riscv_emit::emit_call(out, parent + "_init");
        riscv_emit::emit_empty_line(out);

        riscv_emit::emit_add(out, ArgumentRegister{0}, SavedRegister{1}, ZeroRegister{});

        expression_codegen_.reset_frame();
        expression_codegen_.set_current_class(class_index);
        expression_codegen_.begin_scope();
        expression_codegen_.emit_attributes(out, class_table_->get_attributes(class_index), class_index);
        expression_codegen_.end_scope();
        riscv_emit::emit_empty_line(out);

        // return self
        riscv_emit::emit_add(out, ArgumentRegister{0}, SavedRegister{1}, ZeroRegister{});

        // epilogue: restore s1,
        riscv_emit::emit_load_word(out, SavedRegister{1}, MemoryLocation{-4, FramePointer{}});
        riscv_emit::emit_load_word(out, ReturnAddress{}, MemoryLocation{0, FramePointer{}});
        riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 12);
        riscv_emit::emit_load_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
        riscv_emit::emit_mnemonic(out, Mnemonic::Return);
        riscv_emit::emit_empty_line(out);
    }

    riscv_emit::emit_empty_line(out);
}

void CoolCodegen::emit_class_object_table(ostream &out, vector<string>& class_names) {
    riscv_emit::emit_header_comment(out, "Class Object Table");
    riscv_emit::emit_label(out, "class_objTab");
    
    for (const auto &class_name : class_names) {
        riscv_emit::emit_word(out, class_name + "_protObj");
        riscv_emit::emit_word(out, class_name + "_init");
    }

    riscv_emit::emit_empty_line(out);
}