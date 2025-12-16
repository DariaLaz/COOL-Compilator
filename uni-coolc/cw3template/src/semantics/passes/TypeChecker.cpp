#include "TypeChecker.h"

#include <string>
#include <vector>

#include "CoolParser.h"

using namespace std;

vector<string> TypeChecker::check(CoolParser *parser)
{
    visitProgram(parser->program());
    parser->reset();

    return std::move(errors);
}

std::any TypeChecker::visitClass(CoolParser::ClassContext *ctx)
{
    current_class = ctx->TYPEID(0)->getText();
    visitedMethods.clear();
    return visitChildren(ctx);
}

std::any TypeChecker::visitMethod(CoolParser::MethodContext *ctx)
{
    string methodName = ctx->OBJECTID()->getText();
    string declaredReturnType = ctx->TYPEID()->getText();

    any bodyAny = visit(ctx->expr());

    string bodyType = "any";
    if (bodyAny.has_value() && bodyAny.type() == typeid(string))
        bodyType = any_cast<string>(bodyAny);

    if (bodyType == "self" && declaredReturnType != current_class && !visitedMethods.count(methodName))
    {
        errors.push_back(
            "In class `" + current_class +
            "` method `" + methodName +
            "`: `SELF_TYPE` is not `" + declaredReturnType +
            "`: type of method body is not a subtype of return type");
    }

    visitedMethods.insert(methodName);

    return nullptr;
}

std::any TypeChecker::visitExpr(CoolParser::ExprContext *ctx)
{
    if (!ctx->OBJECTID().empty() && ctx->OBJECTID(0)->getText() == "self")
    {
        return std::any{std::string{"self"}};
    }

    return visitChildren(ctx);
}