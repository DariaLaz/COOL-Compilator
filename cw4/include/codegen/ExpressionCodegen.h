#ifndef CODEGEN_COOL_EXPRESSION_CODEGEN_H_
#define CODEGEN_COOL_EXPRESSION_CODEGEN_H_

#include <ostream>
#include "StaticConstants.h"
#include "Register.h"
#include "semantics/ClassTable.h"
#include "semantics/typed-ast/Expr.h"
#include "semantics/typed-ast/StaticDispatch.h"
#include "semantics/typed-ast/StringConstant.h"
#include "semantics/typed-ast/LetIn.h"
#include  "semantics/typed-ast/NewObject.h"
#include "semantics/typed-ast/DynamicDispatch.h"
#include "semantics/typed-ast/ObjectReference.h"

using namespace std;

class ExpressionCodegen {
  private:
    StaticConstants* static_constants_;
    ClassTable* class_table_;

    void emit_static_dispatch(ostream &out, const StaticDispatch* static_dispatch);
    void emit_string_constant(std::ostream& out, const StringConstant* string_constant);
    void emit_let_in(std::ostream& out, const LetIn* let_in);
    void emit_new_object(std::ostream& out, const NewObject* new_object);
    void emit_dynamic_dispatch(ostream &out, const DynamicDispatch* dynamic_dispatch);
    void emit_object_reference(ostream &out, const ObjectReference* object_reference);
    
  public:
  ExpressionCodegen(StaticConstants* static_constants)
        : static_constants_(static_constants)
        {}
  void setClassTable(ClassTable* class_table) {
      class_table_ = class_table;
  }

  void generate(ostream &out, const Expr* expr);

  private:
  // scopes
  void push_register(ostream &out, Register reg);
  void pop_register(int words_count = 1);
  void pop_words(ostream &out, int words_count);

  int frame_depth_bytes_ = 4;
  std::vector<std::unordered_map<std::string, int>> scopes_;

  void begin_scope();
  void end_scope();
  void bind_var(const std::string& name, int fp_offset);

  int lookup_var(const std::string& name);
public:
  void reset_frame();
};

#endif
