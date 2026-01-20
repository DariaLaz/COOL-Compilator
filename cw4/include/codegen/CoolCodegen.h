#ifndef CODEGEN_COOL_CODEGEN_H_
#define CODEGEN_COOL_CODEGEN_H_

#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "CoolParser.h"
#include "semantics/ClassTable.h"
#include "StaticConstants.h"
#include "ExpressionCodegen.h"

using namespace std;

class CoolCodegen
{
private:
    StaticConstants static_constants_;
    ExpressionCodegen expression_codegen_;

    string file_name_;
    unique_ptr<ClassTable> class_table_;

    void emit_methods(ostream &out);

    void emit_tables(ostream &out);
    void emit_name_table(ostream &out, vector<string> &class_names);
    void emit_className_attributes(ostream &out, const string &class_name);
    void emit_length_attribute(ostream &out, const string &class_name);
    void emit_className(ostream &out, const string &class_name);

    void emit_prototype_tables(ostream &out, vector<string> &class_names);
    void emit_prototype_table(ostream &out, const string &class_name);

    void emit_dispatch_tables(ostream &out, vector<string> &class_names, vector<string> &base_class_names);
    void emit_dispatch_table(ostream &out, const string &class_name, vector<string> &base_class_names);

    void emit_initialization_methods(ostream &out, vector<string> &class_names);

    void emit_class_object_table(ostream &out, vector<string> &class_names);

public:
    CoolCodegen(string file_name, unique_ptr<ClassTable> class_table)
        : file_name_(move(file_name)),
          class_table_(move(class_table)),
          static_constants_(),
          expression_codegen_(&static_constants_)
    {
        expression_codegen_.set_class_table(class_table_.get());
        expression_codegen_.set_file_name(file_name_);
        static_constants_.set_class_table(class_table_.get());
    }

    void generate(ostream &out);
};

#endif
