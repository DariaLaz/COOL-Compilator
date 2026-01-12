#include "ExpressionCodegen.h"

#include "codegen/CodeEmitter.h"

using namespace std;

void ExpressionCodegen::generate(ostream &out, const Expr* expr) {
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

    if (auto sequence = dynamic_cast<const Sequence *>(expr)) {
        return emit_sequence(out, sequence);
    }

    if (auto int_constant = dynamic_cast<const IntConstant *>(expr)) {
        return emit_int_constant(out, int_constant);
    }

    if (auto assignment = dynamic_cast<const Assignment *>(expr)) {
        return emit_assignment(out, assignment);
    }

    if (auto method_invocation = dynamic_cast<const MethodInvocation *>(expr)) {
        return emit_method_invocation(out, method_invocation);
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
    riscv_emit::emit_move(out, TempRegister{0}, ArgumentRegister{0}); // t0 = receiver

    push_register(out, FramePointer{});

    const auto& args = expr->get_arguments();
    for (int i = (int)args.size() - 1; i >= 0; --i) {
        generate(out, args[i]);
        push_register(out, ArgumentRegister{0});
    }

    riscv_emit::emit_move(out, ArgumentRegister{0}, TempRegister{0});

    string class_name(class_table_->get_name(expr->get_static_dispatch_type()));
    string method_name = expr->get_method_name();
    riscv_emit::emit_jump_and_link(out, class_name + "." + method_name);

    pop_register(1 + args.size());
}

void ExpressionCodegen::emit_let_in(std::ostream& out, const LetIn* let_in) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Let In");

    riscv_emit::emit_move(out, TempRegister{2}, ArgumentRegister{0}); // t2 = receiver

    begin_scope();

    for (const auto& vardecl : let_in->get_vardecls()) {
        if (vardecl->has_initializer()) {
            generate(out, vardecl->get_initializer());
        } else {
            string class_name(class_table_->get_name(vardecl->get_type()));
            string label = static_constants_->use_default_value(class_name);
            riscv_emit::emit_load_address(out, ArgumentRegister{0}, label);
        }

        int fp_offset = -frame_depth_bytes_;
        push_register(out, ArgumentRegister{0});
        bind_var(vardecl->get_name(), fp_offset);
    }

    riscv_emit::emit_move(out, ArgumentRegister{0}, TempRegister{2}); // a0 = receiver

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
    pop_register(1);

    push_register(out, FramePointer{});
    riscv_emit::emit_call(out, class_name + "_init");
    pop_register(1);
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
    return -1;
  }

void ExpressionCodegen::reset_frame() {
    frame_depth_bytes_ = 8;
    scopes_.clear();
    begin_scope();
}

// -----------------------

void ExpressionCodegen::emit_dynamic_dispatch(ostream& out, const DynamicDispatch* expr) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Dynamic Dispatch");

    generate(out, expr->get_target());
    riscv_emit::emit_move(out, TempRegister{0}, ArgumentRegister{0}); // t0 = receiver

    push_register(out, FramePointer{});

    const auto& args = expr->get_arguments();
    for (int i = (int)args.size() - 1; i >= 0; --i) {
        generate(out, args[i]);
        push_register(out, ArgumentRegister{0});
    }

    riscv_emit::emit_move(out, ArgumentRegister{0}, TempRegister{0}); // a0 = receiver

    int method_index = class_table_->get_method_index(expr->get_target()->get_type(), expr->get_method_name());
    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{8, ArgumentRegister{0}}); // dispTab
    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{4 * method_index, TempRegister{1}});
    riscv_emit::emit_jump_and_link_register(out, TempRegister{1});

    pop_register(1 + (int)args.size());
}

void ExpressionCodegen::emit_object_reference(ostream& out, const ObjectReference* object_reference) {
     string name = object_reference->get_name();

    if (name == "self") {
        riscv_emit::emit_move(out, ArgumentRegister{0}, SavedRegister{1});
        return;
    }

    int fp_offset = lookup_var(name);

    if (fp_offset != -1) {
        riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{fp_offset, FramePointer{}});
        return;
    }

       auto all_attrs = class_table_->get_all_attributes(current_class_index_);

    int attr_i = -1;
    for (int i = 0; i < (int)all_attrs.size(); ++i) {
        if (all_attrs[i] == name) { attr_i = i; break; }
    }

    if (attr_i < 0) {
        riscv_emit::emit_comment(out, "ICE: unknown identifier");
        riscv_emit::emit_move(out, ArgumentRegister{0}, ZeroRegister{});
        return;
    }

    int byte_offset = (3 + attr_i) * 4; 
    riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{byte_offset, SavedRegister{1}});
}

void ExpressionCodegen::emit_sequence(ostream& out, const Sequence* sequence) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Sequence");
    for (const auto& expr : sequence->get_sequence()) {
        generate(out, expr);
    }
}

void ExpressionCodegen::emit_int_constant(ostream& out, const IntConstant* int_constant) {
    string label = static_constants_->use_int_constant(int_constant->get_value());
    riscv_emit::emit_load_address(out, ArgumentRegister{0}, label);
}

void ExpressionCodegen::emit_assignment(ostream& out, const Assignment* assignment) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Assignment");
    generate(out, assignment->get_value());

    int fp_offset = lookup_var(assignment->get_assignee_name());
    riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{fp_offset, FramePointer{}});
}

void ExpressionCodegen::emit_method_invocation(ostream& out, const MethodInvocation* mi) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Method Invocation");

    const auto& args = mi->get_arguments();
    const int argc = args.size();

    riscv_emit::emit_move(out, TempRegister{0}, SavedRegister{1}); // t0 = self


    push_register(out, FramePointer{});


    for (int i = argc - 1; i >= 0; --i) {
        generate(out, args[i]);
        push_register(out, ArgumentRegister{0});
    }

    riscv_emit::emit_move(out, ArgumentRegister{0}, TempRegister{0}); // a0 = self

    int method_index = class_table_->get_method_index(current_class_index_, mi->get_method_name());
    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{8, ArgumentRegister{0}}); // dispTab
    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{4 * method_index, TempRegister{1}});
    riscv_emit::emit_jump_and_link_register(out, TempRegister{1});

    pop_register(1 + argc);
}

void ExpressionCodegen::emit_attributes(
    ostream &out,
    const vector<string>& attribute_names,
    int class_index
) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Init attributes");

    riscv_emit::emit_move(out, TempRegister{0}, ArgumentRegister{0}); // t0 = self

    auto all_attrs = class_table_->get_all_attributes(class_index);

    const string current_class_name(class_table_->get_name(class_index));

    for (const string& attr_name : attribute_names) {
        int pos = -1;
        for (int i = 0; i < (int)all_attrs.size(); ++i) {
            if (all_attrs[i] == attr_name) { pos = i; break; }
        }
        if (pos < 0) {
            riscv_emit::emit_comment(out, "ICE: attribute not found in layout");
            continue;
        }

        int byte_offset = (3 + pos) * 4;

        riscv_emit::emit_move(out, ArgumentRegister{0}, TempRegister{0});

        const Expr* init =
            class_table_->transitive_get_attribute_initializer(current_class_name, attr_name);

        if (init) {
            generate(out, init);
        } else {
            auto opt_type = class_table_->transitive_get_attribute_type(class_index, attr_name);
            int type_index = opt_type ? *opt_type : 0;
            string type_name(class_table_->get_name(type_index));
            string label = static_constants_->use_default_value(type_name);
            riscv_emit::emit_load_address(out, ArgumentRegister{0}, label);
        }

        riscv_emit::emit_store_word(out, ArgumentRegister{0},
                                    MemoryLocation{byte_offset, TempRegister{0}});
        riscv_emit::emit_empty_line(out);
    }

    riscv_emit::emit_move(out, ArgumentRegister{0}, TempRegister{0});
}

void ExpressionCodegen::bind_formals(const vector<string>& formals) {
    int offset = 4;
    for (const auto& name : formals) {
        bind_var(name, offset);
        offset += 4;
    }
}