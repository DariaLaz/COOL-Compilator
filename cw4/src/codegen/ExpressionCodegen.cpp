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

    riscv_emit::emit_comment(out, "TODO: unsupported expr");
}

void ExpressionCodegen::emit_string_constant(std::ostream& out, const StringConstant* string_constant) {
    string label = static_constants_->use_string_constant(string_constant->get_value());
    riscv_emit::emit_load_address(out, ArgumentRegister{0}, label);
}

void ExpressionCodegen::emit_static_dispatch(ostream& out, const StaticDispatch* expr) {
    riscv_emit::emit_store_word(out, FramePointer{}, MemoryLocation{0, StackPointer{}});
    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);

    for (const auto& argument : expr->get_arguments()) {
        generate(out, argument);
        riscv_emit::emit_store_word(out, ArgumentRegister{0}, MemoryLocation{0, StackPointer{}});
        riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, -4);
    }

    // expression_codegen_.generate(expr->get_target());
    out << "    jal IO.out_string\n";

    riscv_emit::emit_add_immediate(out, StackPointer{}, StackPointer{}, 8);
}