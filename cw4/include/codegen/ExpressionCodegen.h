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
#include "semantics/typed-ast/NewObject.h"
#include "semantics/typed-ast/DynamicDispatch.h"
#include "semantics/typed-ast/ObjectReference.h"
#include "semantics/typed-ast/Sequence.h"
#include "semantics/typed-ast/IntConstant.h"
#include "semantics/typed-ast/Assignment.h"
#include "semantics/typed-ast/MethodInvocation.h"
#include "semantics/typed-ast/IfThenElseFi.h"
#include "semantics/typed-ast/BoolConstant.h"
#include "semantics/typed-ast/IsVoid.h"
#include "semantics/typed-ast/IntegerComparison.h"
#include "semantics/typed-ast/EqualityComparison.h"
#include "semantics/typed-ast/WhileLoopPool.h"
#include "semantics/typed-ast/IntegerNegation.h"
#include "semantics/typed-ast/BooleanNegation.h"
#include "semantics/typed-ast/Arithmetic.h"
#include "semantics/typed-ast/ParenthesizedExpr.h"
#include "semantics/typed-ast/CaseOfEsac.h"

using namespace std;

class ExpressionCodegen {
private:
    StaticConstants* static_constants_;
    ClassTable* class_table_;
    int current_class_index_ = 0;
    string file_name_;

    void emit_static_dispatch(ostream& out, const StaticDispatch* static_dispatch);
    void emit_string_constant(ostream& out, const StringConstant* string_constant);
    void emit_let_in(ostream& out, const LetIn* let_in);
    void emit_new_object(ostream& out, const NewObject* new_object);
    void emit_dynamic_dispatch(ostream& out, const DynamicDispatch* dynamic_dispatch);
    void emit_object_reference(ostream& out, const ObjectReference* object_reference);
    void emit_sequence(ostream& out, const Sequence* sequence);
    void emit_int_constant(ostream& out, const IntConstant* int_constant);
    void emit_assignment(ostream& out, const Assignment* assignment);
    void emit_method_invocation(ostream& out, const MethodInvocation* method_invocation);
    void emit_if_then_else_fi(ostream& out, const IfThenElseFi* if_then_else_fi);
    void emit_bool_constant(ostream& out, const BoolConstant* bool_constant);
    void emit_is_void(ostream& out, const IsVoid* is_void);
    void emit_integer_comparison(ostream& out, const IntegerComparison* integer_comparison);
    void emit_equality_comparison(ostream& out, const EqualityComparison* equality_comparison);
    void emit_while_loop_pool(ostream& out, const WhileLoopPool* while_loop_pool);
    void emit_integer_negation(ostream& out, const IntegerNegation* integer_negation);
    void emit_boolean_negation(ostream& out, const BooleanNegation* boolean_negation);
    void emit_arithmetic(ostream& out, const Arithmetic* arithmetic);
    void emit_parenthesized_expr(ostream& out, const ParenthesizedExpr* parenthesized_expr);
    void emit_case_of_esac(ostream& out, const CaseOfEsac* case_of_esac);
   
    string get_file_name_label() {
        return static_constants_->use_string_constant(file_name_);
    }

    void push_register(ostream& out, Register reg);
    void pop_register(int words_count = 1);
    void pop_words(ostream& out, int words_count);

    int frame_depth_bytes_ = 8;
    vector<unordered_map<string, int>> scopes_;

    void bind_var(const string& name, int fp_offset);
    int lookup_var(const string& name);

public:
    ExpressionCodegen(StaticConstants* static_constants)
        : static_constants_(static_constants) {}

    void set_class_table(ClassTable* class_table) {
        class_table_ = class_table;
    }

    void set_file_name(const string& file_name) {
        file_name_ = file_name;
    }

    void set_current_class(int class_index) {
        current_class_index_ = class_index;
    }

    void generate(ostream& out, const Expr* expr);
    void emit_attributes(ostream& out, const vector<string>& attribute_names, int class_index);
    void bind_formals(const vector<string>& formals);
    void begin_scope();
    void end_scope();
    void reset_frame();
};

#endif
