#ifndef CODEGEN_COOL_STATIC_CONSTANTS_H_
#define CODEGEN_COOL_STATIC_CONSTANTS_H_

#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>

using namespace std;

class StaticConstants {
  private:
    int next_string_id = 0;
    unordered_map<string, string> string_to_label;
    
    bool is_true_used = false;
    bool is_false_used = false;
    
    unordered_map<int, string> int_to_label;
    static string unescape_string_literal(const string& raw);
  public:
    string use_string_constant(const string& str);
    string use_bool_constant(bool value);
    string use_int_constant(int value);
    string use_default_value(string class_name);

    void emit_all(ostream &out);
};

#endif
