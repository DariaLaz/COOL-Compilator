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

class CoolCodegen {
  private:
    StaticConstants static_constants_;
    ExpressionCodegen expression_codegen_;

    std::string file_name_;
    std::unique_ptr<ClassTable> class_table_;

    void emit_methods(std::ostream &out);

    void emit_tables(std::ostream &out);
    void emit_name_table(std::ostream &out,  vector<std::string>& class_names);
    void emit_className_attributes(std::ostream &out, const std::string &class_name);
    void emit_length_attribute(std::ostream &out, const std::string &class_name);
    void emit_className(std::ostream &out, const std::string &class_name);
    
    void emit_prototype_tables(std::ostream &out, vector<std::string>& class_names);
    void emit_prototype_table(std::ostream &out, const std::string& class_name);

    void emit_dispatch_tables(std::ostream &out, vector<std::string>& class_names, vector<std::string>& base_class_names);
    void emit_dispatch_table(std::ostream &out, const std::string& class_name, vector<std::string>& base_class_names);

    void emit_initialization_methods(std::ostream &out, vector<std::string>& class_names);

    void emit_class_object_table(std::ostream &out, vector<std::string>& class_names);
  public:
    CoolCodegen(std::string file_name, std::unique_ptr<ClassTable> class_table)
        : file_name_(std::move(file_name)),
          class_table_(std::move(class_table)),
          static_constants_(),
          expression_codegen_(&static_constants_)
          {
              expression_codegen_.set_class_table(class_table_.get());
              expression_codegen_.set_file_name(file_name_);
              static_constants_.set_class_table(class_table_.get());
          }

    void generate(std::ostream &out);
};

#endif
