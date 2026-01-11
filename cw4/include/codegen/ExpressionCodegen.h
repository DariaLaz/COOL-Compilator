#ifndef CODEGEN_COOL_EXPRESSION_CODEGEN_H_
#define CODEGEN_COOL_EXPRESSION_CODEGEN_H_

#include <ostream>
#include "StaticConstants.h"
#include "semantics/typed-ast/Expr.h"
#include "semantics/typed-ast/StaticDispatch.h"
#include "semantics/typed-ast/StringConstant.h"

using namespace std;

class ExpressionCodegen {
  private:
    StaticConstants* static_constants_;

    void emit_static_dispatch(ostream &out, const StaticDispatch* static_dispatch);
    void emit_string_constant(std::ostream& out, const StringConstant* string_constant);
  public:
  ExpressionCodegen(StaticConstants* static_constants)
        : static_constants_(static_constants) {}

  void generate(ostream &out, const Expr* expr);
};

#endif
