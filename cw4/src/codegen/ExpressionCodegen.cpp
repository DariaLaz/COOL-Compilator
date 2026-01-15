#include "ExpressionCodegen.h"

#include <algorithm>
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

    if (auto if_then_else_fi = dynamic_cast<const IfThenElseFi *>(expr)) {
        return emit_if_then_else_fi(out, if_then_else_fi);
    }

    if (auto bool_constant = dynamic_cast<const BoolConstant *>(expr)) {
        return emit_bool_constant(out, bool_constant);
    }

    if (auto is_void = dynamic_cast<const IsVoid *>(expr)) {
        return emit_is_void(out, is_void);
    }

    if (auto integer_comparison = dynamic_cast<const IntegerComparison *>(expr)) {
        return emit_integer_comparison(out, integer_comparison);
    }

    if (auto equality_comparison = dynamic_cast<const EqualityComparison *>(expr)) {
        return emit_equality_comparison(out, equality_comparison);
    }

    if (auto while_loop_pool = dynamic_cast<const WhileLoopPool *>(expr)) {
        return emit_while_loop_pool(out, while_loop_pool);
    }

    if (auto integer_negation = dynamic_cast<const IntegerNegation *>(expr)) {
        return emit_integer_negation(out, integer_negation);
    }

    if (auto boolean_negation = dynamic_cast<const BooleanNegation *>(expr)) {
        return emit_boolean_negation(out, boolean_negation);
    }

    if (auto arithmetic = dynamic_cast<const Arithmetic *>(expr)) {
        return emit_arithmetic(out, arithmetic);
    }

    if (auto parenthesized_expr = dynamic_cast<const ParenthesizedExpr *>(expr)) {
        return emit_parenthesized_expr(out, parenthesized_expr);
    }

    if (auto case_of_esac = dynamic_cast<const CaseOfEsac *>(expr)) {
        return emit_case_of_esac(out, case_of_esac);
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


    generate(out, expr->get_target());

    riscv_emit::emit_move(out, TempRegister{6}, ArgumentRegister{0});

    push_register(out, FramePointer{});

    const auto& args = expr->get_arguments();
    for (int i = (int)args.size() - 1; i >= 0; --i) {
        generate(out, args[i]);
        push_register(out, ArgumentRegister{0});
    }

    riscv_emit::emit_move(out, ArgumentRegister{0}, TempRegister{6});

    string class_name(class_table_->get_name(expr->get_static_dispatch_type()));
    string method_name = expr->get_method_name();
    riscv_emit::emit_jump_and_link(out, class_name + "." + method_name);
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

            if (!label.empty())
            {riscv_emit::emit_load_address(out, ArgumentRegister{0}, label);}
            else
            {riscv_emit::emit_move(out, ArgumentRegister{0}, ZeroRegister{});}
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
    push_register(out, ArgumentRegister{0});

    push_register(out, FramePointer{});

    const auto& args = expr->get_arguments();
    int argc = args.size();
    for (int i = argc - 1; i >= 0; --i) {
        generate(out, args[i]);
        push_register(out, ArgumentRegister{0});
    }

    riscv_emit::emit_load_word(out, ArgumentRegister{0}, MemoryLocation{4 * (argc + 2), StackPointer{}});

    int method_index = class_table_->get_method_index(expr->get_target()->get_type(), expr->get_method_name());
    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{8, ArgumentRegister{0}});
    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{4 * method_index, TempRegister{1}});
    riscv_emit::emit_jump_and_link_register(out, TempRegister{1});

    pop_register(1 + argc);
    pop_words(out, 1);
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
    if (fp_offset != -1) {
        riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{fp_offset, FramePointer{}});
        return;
    }

    auto all_attrs = class_table_->get_all_attributes(current_class_index_);
    int attr_i = -1;
    for (int i = 0; i < (int)all_attrs.size(); ++i) {
        if (all_attrs[i] == assignment->get_assignee_name()) { attr_i = i; break; }
    }

    if (attr_i < 0) {
        riscv_emit::emit_comment(out, "ICE: assignment to unknown identifier");
        return;
    }

    int byte_offset = (3 + attr_i) * 4;
    riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{byte_offset, SavedRegister{1}});
}

void ExpressionCodegen::emit_method_invocation(ostream& out, const MethodInvocation* mi) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Method Invocation");

    const auto& args = mi->get_arguments();
    const int argc = args.size();

    push_register(out, FramePointer{});

    for (int i = argc - 1; i >= 0; --i) {
        generate(out, args[i]);
        push_register(out, ArgumentRegister{0});
    }

    riscv_emit::emit_move(out, ArgumentRegister{0}, SavedRegister{1});

    int method_index =
        class_table_->get_method_index(current_class_index_, mi->get_method_name());

    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{8, ArgumentRegister{0}});
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


    auto all_attrs = class_table_->get_all_attributes(class_index);
    const string current_class_name(class_table_->get_name(class_index));

    for (const string& attr_name : attribute_names) {
        int pos = -1;
        for (int i = 0; i < (int)all_attrs.size(); ++i) {
            if (all_attrs[i] == attr_name) { pos = i; break; }
        }
        if (pos < 0) continue;

        int byte_offset = (3 + pos) * 4;

        riscv_emit::emit_move(out, ArgumentRegister{0}, SavedRegister{1});

        const Expr* init =
            class_table_->transitive_get_attribute_initializer(current_class_name, attr_name);

        if (init) {
            generate(out, init);
        } else {
            auto opt_type = class_table_->transitive_get_attribute_type(class_index, attr_name);
            int type_index = opt_type ? *opt_type : 0;
            string type_name(class_table_->get_name(type_index));
            string label = static_constants_->use_default_value(type_name);

            if (!label.empty()) riscv_emit::emit_load_address(out, ArgumentRegister{0}, label);
            else riscv_emit::emit_move(out, ArgumentRegister{0}, ZeroRegister{});
        }

        riscv_emit::emit_store_word(out, ArgumentRegister{0},
                                    MemoryLocation{byte_offset, SavedRegister{1}});

        riscv_emit::emit_empty_line(out);
    }

    riscv_emit::emit_move(out, ArgumentRegister{0}, SavedRegister{1});
}

void ExpressionCodegen::bind_formals(const vector<string>& formals) {
    int offset = 4;
    for (const auto& name : formals) {
        bind_var(name, offset);
        offset += 4;
    }
}


void ExpressionCodegen::emit_if_then_else_fi(ostream& out, const IfThenElseFi* if_then_else_fi) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "If Then Else Fi");

    int id = riscv_emit::if_then_else_fi_label_count++;
    string else_lbl = "else_branch_" + to_string(id);
    string fi_lbl   = "fi_end_" + to_string(id);

    generate(out, if_then_else_fi->get_condition());

    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{12, ArgumentRegister{0}});
    riscv_emit::emit_branch_equal_zero(out, TempRegister{1}, else_lbl);

    generate(out, if_then_else_fi->get_then_expr());
    riscv_emit::emit_jump(out, fi_lbl);

    riscv_emit::emit_label(out, else_lbl);
    generate(out, if_then_else_fi->get_else_expr());

    riscv_emit::emit_label(out, fi_lbl);
}

void ExpressionCodegen::emit_bool_constant(ostream& out, const BoolConstant* bool_constant) {
    string label = static_constants_->use_bool_constant(bool_constant->get_value());
    riscv_emit::emit_load_address(out, ArgumentRegister{0}, label);
}

// TODO fix
void ExpressionCodegen::emit_is_void(std::ostream& out, const IsVoid* is_void) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "IsVoid");

    generate(out, is_void->get_subject());

    int id = riscv_emit::if_then_else_fi_label_count++;
    std::string true_lbl = "isvoid_true_" + std::to_string(id);
    std::string end_lbl  = "isvoid_end_"  + std::to_string(id);

    riscv_emit::emit_branch_equal_zero(out, ArgumentRegister{0}, true_lbl);

    riscv_emit::emit_load_address(out, ArgumentRegister{0},
                                 static_constants_->use_bool_constant(false));
    riscv_emit::emit_jump(out, end_lbl);

    riscv_emit::emit_label(out, true_lbl);
    riscv_emit::emit_load_address(out, ArgumentRegister{0},
                                 static_constants_->use_bool_constant(true));

    riscv_emit::emit_label(out, end_lbl);
}

void ExpressionCodegen::emit_integer_comparison(
    std::ostream& out,
    const IntegerComparison* integer_comparison
) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Integer Comparison");

    generate(out, integer_comparison->get_lhs());
    push_register(out, ArgumentRegister{0}); 

    generate(out, integer_comparison->get_rhs());
    riscv_emit::emit_move(out, TempRegister{1}, ArgumentRegister{0});

    pop_words(out, 1);
    riscv_emit::emit_load_word(out, TempRegister{0}, MemoryLocation{0, StackPointer{}});

    riscv_emit::emit_load_word(out, TempRegister{0}, MemoryLocation{12, TempRegister{0}});
    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{12, TempRegister{1}});

    switch (integer_comparison->get_kind()) {
        case IntegerComparison::Kind::LessThan: {
            riscv_emit::emit_set_less_than(out, TempRegister{2}, TempRegister{0}, TempRegister{1});
            break;
        }
        case IntegerComparison::Kind::LessThanEqual: {
            riscv_emit::emit_set_less_than(out, TempRegister{2}, TempRegister{1}, TempRegister{0});
            riscv_emit::emit_xor_immediate(out, TempRegister{2}, TempRegister{2}, 1);
            break;
        }
        default:
            break;
    }

    int id = riscv_emit::if_then_else_fi_label_count++;
    std::string false_lbl = "int_comp_false_" + std::to_string(id);
    std::string end_lbl   = "int_comp_end_"   + std::to_string(id);

    riscv_emit::emit_branch_equal_zero(out, TempRegister{2}, false_lbl);
    riscv_emit::emit_load_address(out, ArgumentRegister{0}, static_constants_->use_bool_constant(true));
    riscv_emit::emit_jump(out, end_lbl);

    riscv_emit::emit_label(out, false_lbl);
    riscv_emit::emit_load_address(out, ArgumentRegister{0}, static_constants_->use_bool_constant(false));

    riscv_emit::emit_label(out, end_lbl);
}

void ExpressionCodegen::emit_equality_comparison(
    std::ostream& out,
    const EqualityComparison* equality_comparison
) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Equality Comparison");

    // lhs
    generate(out, equality_comparison->get_lhs());
    push_register(out, ArgumentRegister{0}); // push lhs obj ptr

    // rhs
    generate(out, equality_comparison->get_rhs());
    riscv_emit::emit_move(out, TempRegister{1}, ArgumentRegister{0}); // t1 = rhs

    // load lhs from stack, then pop it
    riscv_emit::emit_load_word(out, TempRegister{0}, MemoryLocation{4, StackPointer{}}); // t0 = lhs
    pop_words(out, 1); // sp += 4

    int id = riscv_emit::if_then_else_fi_label_count++;
    std::string ret_true   = "eq_true_"  + std::to_string(id);
    std::string ret_false  = "eq_false_" + std::to_string(id);
    std::string end_lbl    = "eq_end_"   + std::to_string(id);

    std::string lhs_void   = "eq_lhs_void_" + std::to_string(id);
    std::string after_void = "eq_after_void_" + std::to_string(id);

    std::string check_int    = "eq_check_int_"    + std::to_string(id);
    std::string check_bool   = "eq_check_bool_"   + std::to_string(id);
    std::string check_string = "eq_check_string_" + std::to_string(id);

    // pointer equality => true
    out << "    sub t4, t0, t1\n";
    out << "    seqz t4, t4\n";
    out << "    bnez t4, " << ret_true << "\n";

    // void handling
    out << "    beqz t0, " << lhs_void << "\n";
    out << "    beqz t1, " << ret_false << "\n";
    out << "    j " << after_void << "\n";

    out << lhs_void << ":\n";
    out << "    beqz t1, " << ret_true << "\n";
    out << "    j " << ret_false << "\n";

    out << after_void << ":\n";

    // require same dynamic type (tag)
    out << "    lw t2, 0(t0)\n"; // t2 = tag(lhs)
    out << "    lw t3, 0(t1)\n"; // t3 = tag(rhs)
    out << "    sub t4, t2, t3\n";
    out << "    bnez t4, " << ret_false << "\n";

    // check by tag
    int int_tag    = class_table_->get_index("Int");
    int bool_tag   = class_table_->get_index("Bool");
    int string_tag = class_table_->get_index("String");

    out << "    li t4, " << int_tag << "\n";
    out << "    sub t4, t2, t4\n";
    out << "    beqz t4, " << check_int << "\n";

    out << "    li t4, " << bool_tag << "\n";
    out << "    sub t4, t2, t4\n";
    out << "    beqz t4, " << check_bool << "\n";

    out << "    li t4, " << string_tag << "\n";
    out << "    sub t4, t2, t4\n";
    out << "    beqz t4, " << check_string << "\n";

    // other objects: only pointer-eq counts (already checked) => false
    out << "    j " << ret_false << "\n";

    // ---- Int compare ----
    out << check_int << ":\n";
    out << "    lw t5, 12(t0)\n";
    out << "    lw t6, 12(t1)\n";
    out << "    sub t4, t5, t6\n";
    out << "    beqz t4, " << ret_true << "\n";
    out << "    j " << ret_false << "\n";

    // ---- Bool compare ----
    out << check_bool << ":\n";
    out << "    lw t5, 12(t0)\n";
    out << "    lw t6, 12(t1)\n";
    out << "    sub t4, t5, t6\n";
    out << "    beqz t4, " << ret_true << "\n";
    out << "    j " << ret_false << "\n";

    // ---- String compare (length + bytes) ----
    out << check_string << ":\n";
    // length Int objects at 12(String), their int values at 12(Int)
    out << "    lw t5, 12(t0)\n";   // lenObjL
    out << "    lw t6, 12(t1)\n";   // lenObjR
    out << "    lw t5, 12(t5)\n";   // lenL
    out << "    lw t6, 12(t6)\n";   // lenR
    out << "    sub t4, t5, t6\n";
    out << "    bnez t4, " << ret_false << "\n";

    // use t2 as remaining count (NO t7!)
    out << "    add t2, t5, zero\n";  // t2 = len

    // ptrs to first char (String header is 4 words => +16)
    out << "    addi t5, t0, 16\n";   // pL
    out << "    addi t6, t1, 16\n";   // pR

    std::string loop_lbl = "eq_str_loop_" + std::to_string(id);
    std::string ok_lbl   = "eq_str_ok_"   + std::to_string(id);

    out << loop_lbl << ":\n";
    out << "    beqz t2, " << ok_lbl << "\n";
    out << "    lb t3, 0(t5)\n";
    out << "    lb t4, 0(t6)\n";
    out << "    bne t3, t4, " << ret_false << "\n";
    out << "    addi t5, t5, 1\n";
    out << "    addi t6, t6, 1\n";
    out << "    addi t2, t2, -1\n";
    out << "    j " << loop_lbl << "\n";

    out << ok_lbl << ":\n";
    out << "    j " << ret_true << "\n";

    // ---- return blocks ----
    out << ret_true << ":\n";
    riscv_emit::emit_load_address(out, ArgumentRegister{0},
                                  static_constants_->use_bool_constant(true));
    riscv_emit::emit_jump(out, end_lbl);

    out << ret_false << ":\n";
    riscv_emit::emit_load_address(out, ArgumentRegister{0},
                                  static_constants_->use_bool_constant(false));

    out << end_lbl << ":\n";
}

void ExpressionCodegen::emit_while_loop_pool(ostream& out, const WhileLoopPool* w) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "While Loop");

    int id = riscv_emit::while_loop_pool_label_count++;
    string begin_lbl = "while_begin_" + to_string(id);
    string end_lbl   = "while_end_"   + to_string(id);

    riscv_emit::emit_label(out, begin_lbl);

    generate(out, w->get_condition());

    riscv_emit::emit_load_word(out, TempRegister{0}, MemoryLocation{12, ArgumentRegister{0}});

    riscv_emit::emit_branch_equal_zero(out, TempRegister{0}, end_lbl);

    generate(out, w->get_body());

    riscv_emit::emit_jump(out, begin_lbl);
    riscv_emit::emit_label(out, end_lbl);

    riscv_emit::emit_move(out, ArgumentRegister{0}, ZeroRegister{});
}

void ExpressionCodegen::emit_integer_negation(
    std::ostream& out,
    const IntegerNegation* integer_negation
) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Integer Negation");

    generate(out, integer_negation->get_argument());

    riscv_emit::emit_load_word(out, TempRegister{3}, MemoryLocation{12, ArgumentRegister{0}});
    riscv_emit::emit_subtract(out, TempRegister{3}, ZeroRegister{}, TempRegister{3});

    riscv_emit::emit_load_address(out, ArgumentRegister{0}, "Int_protObj");
    push_register(out, FramePointer{});
    riscv_emit::emit_call(out, "Object.copy");
    pop_register(1);

    riscv_emit::emit_store_word(out, TempRegister{3}, MemoryLocation{12, ArgumentRegister{0}});
}

void ExpressionCodegen::emit_boolean_negation(
    std::ostream& out,
    const BooleanNegation* boolean_negation
) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Boolean Negation");

    generate(out, boolean_negation->get_argument());

    riscv_emit::emit_load_word(out, TempRegister{3}, MemoryLocation{12, ArgumentRegister{0}});
    riscv_emit::emit_xor_immediate(out, TempRegister{3}, TempRegister{3}, 1);

    riscv_emit::emit_load_address(out, ArgumentRegister{0}, "Bool_protObj");
    push_register(out, FramePointer{});
    riscv_emit::emit_call(out, "Object.copy");
    pop_register(1);

    riscv_emit::emit_store_word(out, TempRegister{3}, MemoryLocation{12, ArgumentRegister{0}});
}

void ExpressionCodegen::emit_arithmetic(std::ostream& out, const Arithmetic* arithmetic) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Arithmetic");

    generate(out, arithmetic->get_lhs());
    push_register(out, ArgumentRegister{0});

    generate(out, arithmetic->get_rhs());
    riscv_emit::emit_move(out, TempRegister{3}, ArgumentRegister{0}); 

    riscv_emit::emit_load_word(out, TempRegister{4}, MemoryLocation{4, StackPointer{}}); 
    pop_words(out, 1);

    riscv_emit::emit_load_word(out, TempRegister{3}, MemoryLocation{12, TempRegister{3}});
    riscv_emit::emit_load_word(out, TempRegister{4}, MemoryLocation{12, TempRegister{4}});

    switch (arithmetic->get_kind()) {
        case Arithmetic::Kind::Addition:
            riscv_emit::emit_add(out, SavedRegister{2}, TempRegister{4}, TempRegister{3});
            break;
        case Arithmetic::Kind::Subtraction:
            riscv_emit::emit_subtract(out, SavedRegister{2}, TempRegister{4}, TempRegister{3});
            break;
        case Arithmetic::Kind::Multiplication:
            riscv_emit::emit_multiply(out, SavedRegister{2}, TempRegister{4}, TempRegister{3});
            break;
        case Arithmetic::Kind::Division:
            riscv_emit::emit_divide(out, SavedRegister{2}, TempRegister{4}, TempRegister{3});
            break;
        default:
            break;
    }

    riscv_emit::emit_load_address(out, ArgumentRegister{0}, "Int_protObj");
    push_register(out, FramePointer{});
    riscv_emit::emit_call(out, "Object.copy");
    pop_register(1);

    riscv_emit::emit_store_word(out, SavedRegister{2}, MemoryLocation{12, ArgumentRegister{0}});
}

void ExpressionCodegen::emit_parenthesized_expr(
    std::ostream& out,
    const ParenthesizedExpr* parenthesized_expr
) {
    generate(out, parenthesized_expr->get_contents());
}

void ExpressionCodegen::emit_case_of_esac(std::ostream& out, const CaseOfEsac* e) {
    riscv_emit::emit_empty_line(out);
    riscv_emit::emit_comment(out, "Case Of Esac");

    generate(out, e->get_multiplex());
    riscv_emit::emit_move(out, TempRegister{0}, ArgumentRegister{0}); // t0 = obj

    int id = riscv_emit::case_of_esac_count++;
    std::string end_lbl      = "case_end_" + std::to_string(id);
    std::string void_lbl     = "case_void_" + std::to_string(id);
    std::string no_match_lbl = "case_no_match_" + std::to_string(id);

    riscv_emit::emit_branch_equal_zero(out, TempRegister{0}, void_lbl);

    riscv_emit::emit_load_word(out, TempRegister{1}, MemoryLocation{0, TempRegister{0}}); // t1 = tag

    vector<const CaseOfEsac::Case*> cases;
    cases.reserve(e->get_cases().size());
    for (const auto& cs : e->get_cases()) cases.push_back(&cs);

    sort(cases.begin(), cases.end(),
        [this](const CaseOfEsac::Case* a, const CaseOfEsac::Case* b) {
            int ta = a->get_type();
            int tb = b->get_type();
            if (ta == tb) return false;

            bool a_sub_b = class_table_->is_subclass_of(ta, tb);
            bool b_sub_a = class_table_->is_subclass_of(tb, ta);

            if (a_sub_b != b_sub_a) return a_sub_b;
            return ta < tb;
        });

    for (const auto* cs : cases) {
        int T = cs->get_type();

        string br_lbl   = "case_branch_" + to_string(id) + "_" + to_string(T);
        string next_lbl = br_lbl + "_next";

        int min_tag = T, max_tag = T;
        bool first = true;

        // TODO: check
        int n = class_table_->get_num_of_classes();
        for (int c = 0; c < n; ++c) {
            if (class_table_->is_subclass_of(c, T)) {
                if (first) { min_tag = max_tag = c; first = false; }
                else { min_tag = min(min_tag, c); max_tag = max(max_tag, c); }
            }
        }

        riscv_emit::emit_add_immediate(out, TempRegister{2}, ZeroRegister{}, min_tag);
        riscv_emit::emit_subtract(out, TempRegister{3}, TempRegister{1}, TempRegister{2}); 
        riscv_emit::emit_branch_less_than_zero(out, TempRegister{3}, next_lbl);

        riscv_emit::emit_add_immediate(out, TempRegister{2}, ZeroRegister{}, max_tag);
        riscv_emit::emit_subtract(out, TempRegister{3}, TempRegister{1}, TempRegister{2});
        riscv_emit::emit_branch_greater_than_zero(out, TempRegister{3}, next_lbl);

        riscv_emit::emit_jump(out, br_lbl);
        riscv_emit::emit_label(out, next_lbl);
    }

    // no match -> abort
    riscv_emit::emit_jump(out, no_match_lbl);

    for (const auto* cs : cases) {
        int T = cs->get_type();
        std::string br_lbl = "case_branch_" + std::to_string(id) + "_" + std::to_string(T);
        riscv_emit::emit_label(out, br_lbl);

        begin_scope();

        int fp_offset = -frame_depth_bytes_;
        riscv_emit::emit_move(out, ArgumentRegister{0}, TempRegister{0});
        push_register(out, ArgumentRegister{0});
        bind_var(cs->get_name(), fp_offset);

        generate(out, cs->get_expr());

        pop_words(out, 1);
        end_scope();

        riscv_emit::emit_jump(out, end_lbl);
    }

    // void
    riscv_emit::emit_label(out, void_lbl);

    riscv_emit::emit_move(out, ArgumentRegister{0}, SavedRegister{1}); 
    push_register(out, FramePointer{});

    riscv_emit::emit_load_address(out, TempRegister{2}, get_file_name_label());
    push_register(out, TempRegister{2});

    riscv_emit::emit_load_address(out, TempRegister{2}, static_constants_ ->use_int_constant(e->get_line()));
    push_register(out, TempRegister{2});

    riscv_emit::emit_call(out, "_case_abort_on_void");

    riscv_emit::emit_jump(out, end_lbl);

    // no match
    riscv_emit::emit_label(out, no_match_lbl);
    riscv_emit::emit_move(out, ArgumentRegister{0}, SavedRegister{1});

    push_register(out, FramePointer{});

    riscv_emit::emit_load_address(out, TempRegister{2}, get_file_name_label());
    push_register(out, TempRegister{2});

    riscv_emit::emit_load_address(out, TempRegister{2}, static_constants_ ->use_int_constant(e->get_line()));
    push_register(out, TempRegister{2});

    riscv_emit::emit_load_address(out, TempRegister{2}, "class_nameTab");             
    riscv_emit::emit_shift_left_immediate(out, TempRegister{3}, TempRegister{1}, 2); 
    riscv_emit::emit_add(out, TempRegister{2}, TempRegister{2}, TempRegister{3});    
    riscv_emit::emit_load_word(out, TempRegister{2}, MemoryLocation{0, TempRegister{2}}); 
    push_register(out, TempRegister{2});
    riscv_emit::emit_call(out, "_case_abort_no_match");
    riscv_emit::emit_jump(out, end_lbl);

    // end
    riscv_emit::emit_label(out, end_lbl);
}