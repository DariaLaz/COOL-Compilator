#ifndef CODEGEN_COOL_CODEGEN_H_
#define CODEGEN_COOL_CODEGEN_H_

#include <memory>
#include <ostream>

#include "CoolParser.h"
#include "semantics/ClassTable.h"

class CoolCodegen {
  private:
    std::string file_name_;
    std::unique_ptr<ClassTable> class_table_;

    void emit_name_table(std::ostream &out);
    void emit_className_attributes(std::ostream &out, const std::string &class_name);
    void emit_length_attribute(std::ostream &out, const std::string &class_name);
    void emit_className(std::ostream &out, const std::string &class_name);

  public:
    CoolCodegen(std::string file_name, std::unique_ptr<ClassTable> class_table)
        : file_name_(std::move(file_name)),
          class_table_(std::move(class_table)){
          }

    void generate(std::ostream &out);
};

#endif
