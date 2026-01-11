#include "ExpressionCodegen.h"

#include "codegen/CodeEmitter.h"

using namespace std;

void ExpressionCodegen::generate(ostream &out, const Expr* expr) {
    // if (auto assignment = dynamic_cast<const Assignment *>(expr)) {
    //     return emit_assignment(out, assignment);
    // }
    if (auto static_dispatch = dynamic_cast<const StaticDispatch *>(expr)) {
        return emit_static_dispatch(out, static_dispatch);
    } 

    if (auto string_constant = dynamic_cast<const StringConstant *>(expr)) {
        return emit_string_constant(out, string_constant);
    }

    if (auto let_in = dynamic_cast<const LetIn *>(expr)) {
        return emit_let_in(out, let_in);
    }

    if (auto new_object = dynamic_cast<const NewObject *>(expr)) {
        return emit_new_object(out, new_object);
    }

    if (auto dynamic_dispatch = dynamic_cast<const DynamicDispatch *>(expr)) {
        return emit_dynamic_dispatch(out, dynamic_dispatch);
    }

    if (auto object_reference = dynamic_cast<const ObjectReference *>(expr)) {
        return emit_object_reference(out, object_reference);
    }

    riscv_emit::emit_comment(out, "TODO: unsupported expr");
}

void ExpressionCodegen::emit_string_constant(std::ostream& out, const StringConstant* string_constant) {
    string label = static_constants_->use_string_constant(string_constant->get_value());
    riscv_emit::emit_load_address(out, ArgumentRegister{0}, label);
}

void ExpressionCodegen::emit_static_dispatch(ostream& out, const StaticDispatch* expr) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Static Dispatch");
    push_register(out, FramePointer{});

    int argc = 0;
    for (const auto& argument : expr->get_arguments()) {
        generate(out, argument);
        push_register(out, ArgumentRegister{0});
        ++argc;
    }

    riscv_emit::emit_jump_and_link(out, "IO.out_string");

    pop_register(argc + 1);
}

void ExpressionCodegen::emit_let_in(std::ostream& out, const LetIn* let_in) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Let In");
    begin_scope();

    for (const auto& vardecl : let_in->get_vardecls()) {
        if (vardecl->get_initializer()) {
            generate(out, vardecl->get_initializer());
        } else {
            riscv_emit::emit_move(out, ArgumentRegister{0}, ZeroRegister{});
        }

        int fp_offset = -frame_depth_bytes_;
        push_register(out, ArgumentRegister{0});
        bind_var(vardecl->get_name(), fp_offset);
    }

    generate(out, let_in->get_body());

    riscv_emit::emit_empty_line(out);
    pop_words(out, let_in->get_vardecls().size());
    end_scope();
}

void ExpressionCodegen::emit_new_object(std::ostream& out, const NewObject* new_object) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "New Object");
    string class_name(class_table_->get_name(new_object->get_type()));

    riscv_emit::emit_load_address(out, ArgumentRegister{0}, class_name + "_protObj");
    
    push_register(out, FramePointer{});
    riscv_emit::emit_call(out, "Object.copy");
    pop_register();

    push_register(out, FramePointer{});
    riscv_emit::emit_call(out, class_name + "_init");
    pop_register();
}

// UTILS

void ExpressionCodegen::push_register(ostream &out, Register reg) {
    riscv_emit::emit_store_word(out, reg, MemoryLocation{0, StackPointer{}});
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
    frame_depth_bytes_ += 4;
}
void ExpressionCodegen::pop_register(int words_count) {
    frame_depth_bytes_ -= 4 * words_count;
}

void ExpressionCodegen::pop_words(ostream& out, int words_count) {
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 4 * words_count);
    frame_depth_bytes_ -= 4 * words_count;
}

void ExpressionCodegen::bind_var(const std::string& name, int fp_offset) {
    scopes_.back()[name] = fp_offset;
}

void ExpressionCodegen::begin_scope() { scopes_.push_back({}); }
void ExpressionCodegen::end_scope() { scopes_.pop_back(); }

int ExpressionCodegen::lookup_var(const std::string& name){
    for (int i = (int)scopes_.size() - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name);
        if (it != scopes_[i].end()) return it->second;
    }
    return 0;
  }

void ExpressionCodegen::reset_frame() {
    frame_depth_bytes_ = 4;
    scopes_.clear();
    begin_scope();
}

// -----------------------

void ExpressionCodegen::emit_dynamic_dispatch(ostream& out, const DynamicDispatch* expr) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Dynamic Dispatch");
    generate(out, expr->get_target());

    // save receiver
    riscv_emit::emit_move(out, TempRegister{0}, ArgumentRegister{0});

    push_register(out, FramePointer{});

    const auto& args = expr->get_arguments();
    for (int i = (int)args.size() - 1; i >= 0; --i) {
        generate(out, args[i]); 
        push_register(out, ArgumentRegister{0});
    }

    riscv_emit::emit_move(out, ArgumentRegister{0}, TempRegister{0});

    int method_index = class_table_->get_method_index(expr->get_target()->get_type(), expr->get_method_name());

    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{8, ArgumentRegister{0}});

    riscv_emit::emit_load_word(out, TempRegister{1},
                               MemoryLocation{4 * method_index, TempRegister{1}});

    riscv_emit::emit_jump_and_link_register(out, TempRegister{1});

    pop_register(1 + args.size());
}

void ExpressionCodegen::emit_object_reference(ostream& out, const ObjectReference* object_reference) {
    int fp_offset = lookup_var(object_reference->get_name());
    riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{fp_offset, FramePointer{}});
}