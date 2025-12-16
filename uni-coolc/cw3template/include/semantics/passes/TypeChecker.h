#ifndef SEMANTICS_PASSES_TYPE_CHECKER_H_
#define SEMANTICS_PASSES_TYPE_CHECKER_H_

#include <any>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"

using namespace std;

class TypeChecker : public CoolParserBaseVisitor
{
private:
  // define any necessary fields
  std::vector<std::string> errors;

  // override necessary visitor methods

  // define helper methods
  std::string current_class;
  std::unordered_set<std::string> visitedMethods;

  unordered_map<string, CoolParser::ClassContext *> classes;
  unordered_map<string, string> parent;
  vector<string> classesInOrder;

public:
  // TODO: add necessary dependencies to constructor
  TypeChecker(
      unordered_map<string, CoolParser::ClassContext *> &classes,
      unordered_map<string, string> &parent,
      vector<string> &classesInOrder)
  {
    this->classes = classes;
    this->parent = parent;
    this->classesInOrder = classesInOrder;
  }

  // Typechecks the AST that the parser produces and returns a list of errors,
  // if any.
  std::vector<std::string> check(CoolParser *parser);

  std::any visitClass(CoolParser::ClassContext *ctx) override;
  std::any visitMethod(CoolParser::MethodContext *ctx) override;
  std::any visitExpr(CoolParser::ExprContext *ctx) override;
};

#endif
