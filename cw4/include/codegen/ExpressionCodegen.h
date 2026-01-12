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
#include "semantics/typed-ast/Sequence.h"
#include "semantics/typed-ast/IntConstant.h"
#include "semantics/typed-ast/Assignment.h"
#include "semantics/typed-ast/MethodInvocation.h"


using namespace std;

class ExpressionCodegen {
  private:
    StaticConstants* static_constants_;
    ClassTable* class_table_;
    int current_class_index_ = 0;

    void emit_static_dispatch(ostream &out, const StaticDispatch* static_dispatch);
    void emit_string_constant(std::ostream& out, const StringConstant* string_constant);
    void emit_let_in(std::ostream& out, const LetIn* let_in);
    void emit_new_object(std::ostream& out, const NewObject* new_object);
    void emit_dynamic_dispatch(ostream &out, const DynamicDispatch* dynamic_dispatch);
    void emit_object_reference(ostream &out, const ObjectReference* object_reference);
    void emit_sequence(ostream &out, const Sequence* sequence);
    void emit_int_constant(ostream &out, const IntConstant* int_constant);
    void emit_assignment(ostream &out, const Assignment* assignment);
    void emit_method_invocation(ostream &out, const MethodInvocation* method_invocation);
    
  public:

  void set_current_class(int class_index) {
      current_class_index_ = class_index;
  }
  void emit_attributes(ostream &out, const vector<string>& attribute_names, int class_index);

  ExpressionCodegen(StaticConstants* static_constants)
        : static_constants_(static_constants)
        {}
  void setClassTable(ClassTable* class_table) {
      class_table_ = class_table;
  }

  void generate(ostream &out, const Expr* expr);

  void bind_formals(const vector<string>& formals);
   void begin_scope();
  void end_scope();


  private:
  // scopes
  void push_register(ostream &out, Register reg);
  void pop_register(int words_count = 1);
  void pop_words(ostream &out, int words_count);

  int frame_depth_bytes_ = 8;
  std::vector<std::unordered_map<std::string, int>> scopes_;

 
  void bind_var(const std::string& name, int fp_offset);

  int lookup_var(const std::string& name);
public:
  void reset_frame();
};

#endif
