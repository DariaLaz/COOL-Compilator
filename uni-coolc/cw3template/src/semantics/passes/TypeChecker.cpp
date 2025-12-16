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
    attrTypes.clear();
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

    if (!classes.count(declaredReturnType))
    {
        errors.push_back("Method `" + methodName + "` in class `" + current_class + "` declared to have return type `" + declaredReturnType + "` which is undefined");
    }
    else

        if (bodyType == "SELF_TYPE" && !visitedMethods.count(methodName))
    {
        unordered_set<string> possibleSelfTypes;
        unordered_set<string> visitedP;

        possibleSelfTypes.insert(current_class);
        possibleSelfTypes.insert("Object");

        string current = parent.at(current_class);
        while (classes.count(current))
        {
            if (visitedP.count(current))
            {
                break;
            }
            visitedP.insert(current);

            possibleSelfTypes.insert(current);
            current = parent.at(current);
        }

        if (!possibleSelfTypes.count(declaredReturnType))
        {
            errors.push_back(
                "In class `" + current_class +
                "` method `" + methodName +
                "`: `SELF_TYPE` is not `" + declaredReturnType +
                "`: type of method body is not a subtype of return type");
        }
    }

    visitedMethods.insert(methodName);

    return nullptr;
}

std::any TypeChecker::visitExpr(CoolParser::ExprContext *ctx)
{
    if (ctx->STR_CONST())
        return std::any{std::string{"String"}};
    if (ctx->INT_CONST())
        return std::any{std::string{"Int"}};
    if (ctx->BOOL_CONST())
        return std::any{std::string{"Bool"}};

    if (ctx->ASSIGN())
    {
        std::string rhsType = "any";
        any anyRhsType = visit(ctx->expr(0));
        if (anyRhsType.has_value() && anyRhsType.type() == typeid(string))
            rhsType = any_cast<string>(anyRhsType);

        std::string lhsName = ctx->OBJECTID(0)->getText();
        std::string lhsType = attrTypes[lhsName];

        if (rhsType != lhsType)
        {
            errors.push_back(
                "In class `" + current_class +
                "` assignee `" + lhsName +
                "`: `" + rhsType +
                "` is not `" + lhsType +
                "`: type of initialization expression is not a subtype of object type");
        }

        return lhsType;
    }

    if (!ctx->OBJECTID().empty() && ctx->OBJECTID(0)->getText() == "self")
    {
        return std::any{std::string{"SELF_TYPE"}};
    }

    return visitChildren(ctx);
}

std::any TypeChecker::visitAttr(CoolParser::AttrContext *ctx)
{
    std::string attrName = ctx->OBJECTID()->getText();
    std::string declaredType = ctx->TYPEID()->getText();

    if (declaredType != "Int" &&
        declaredType != "Bool" &&
        declaredType != "String" &&
        declaredType != "Object" &&
        !classes.count(declaredType))
    {
        errors.push_back(
            "Attribute `" + attrName + "` in class `" + current_class +
            "` declared to have type `" + declaredType + "` which is undefined");
        return nullptr;
    }

    if (attrTypes.count(attrName))
    {
        errors.push_back(
            "Attribute `" + attrName + "` already defined for class `" + current_class + "`");
        return nullptr;
    }
    attrTypes[attrName] = declaredType;

    if (ctx->expr())
    {
        auto initTypeAny = visit(ctx->expr());
        if (initTypeAny.has_value())
        {
            std::string initType = std::any_cast<std::string>(initTypeAny);
            if (initType != declaredType && initType != "SELF_TYPE")
            {
                errors.push_back(
                    "In class `" + current_class + "` attribute `" + attrName +
                    "`: `" + initType + "` is not `" + declaredType +
                    "`: type of initialization expression is not a subtype of declared type");
            }
        }
    }

    return nullptr;
}
