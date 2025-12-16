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
    collectAttributes(ctx);

    methodReturnTypes[current_class].clear();
    for (auto *m : ctx->method())
    {
        string name = m->OBJECTID()->getText();
        string returnT = m->TYPEID()->getText();
        methodReturnTypes[current_class][name] = returnT;
    }

    return visitChildren(ctx);
}

void TypeChecker::collectAttributes(CoolParser::ClassContext *ctx)
{
    attrTypes.clear();

    for (auto *attr : ctx->attr())
    {
        string attrName = attr->OBJECTID()->getText();
        string declaredType = attr->TYPEID()->getText();

        if (declaredType != "Int" &&
            declaredType != "Bool" &&
            declaredType != "String" &&
            declaredType != "Object" &&
            !classes.count(declaredType))
        {
            errors.push_back(
                "Attribute `" + attrName + "` in class `" + current_class +
                "` declared to have type `" + declaredType + "` which is undefined");
            continue;
        }

        if (attrTypes.count(attrName))
        {
            errors.push_back(
                "Attribute `" + attrName + "` already defined for class `" + current_class + "`");
            continue;
        }

        attrTypes[attrName] = declaredType;
    }
}

std::any TypeChecker::visitMethod(CoolParser::MethodContext *ctx)
{
    string methodName = ctx->OBJECTID()->getText();
    string declaredReturnType = ctx->TYPEID()->getText();

    any bodyAny = visit(ctx->expr());

    string bodyType = "__ERROR";

    if (bodyAny.has_value() && bodyAny.type() == typeid(string))
        bodyType = any_cast<string>(bodyAny);

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
    // maybe use everywhere
    bool isBuiltin =
        declaredReturnType == "Int" ||
        declaredReturnType == "Bool" ||
        declaredReturnType == "String" ||
        declaredReturnType == "Object" ||
        declaredReturnType == "SELF_TYPE";

    if (!isBuiltin && !classes.count(declaredReturnType))
    {
        errors.push_back("Method `" + methodName + "` in class `" + current_class + "` declared to have return type `" + declaredReturnType + "` which is undefined");
    }
    else

        if (bodyType == "SELF_TYPE" && !visitedMethods.count(methodName))
    {
        if (!possibleSelfTypes.count(declaredReturnType))
        {
            errors.push_back(
                "In class `" + current_class +
                "` method `" + methodName +
                "`: `SELF_TYPE` is not `" + declaredReturnType +
                "`: type of method body is not a subtype of return type");
        }
    }
    else if (bodyType != declaredReturnType && bodyType != "__ERROR")
    {
        if (bodyType != "SELF_TYPE" && !possibleSelfTypes.count(bodyType) && !possibleSelfTypes.count(declaredReturnType))
            errors.push_back(
                "In class `" + current_class +
                "` method `" + methodName +
                "`: `" + bodyType +
                "` is not `" + declaredReturnType +
                "`: type of method body is not a subtype of return type");
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
        std::string lhsName = ctx->OBJECTID(0)->getText();

        if (!attrTypes.count(lhsName))
        {
            errors.push_back(
                "Assignee named `" + lhsName + "` not in scope");

            return std::any{std::string{"__ERROR"}};
        }

        std::string rhsType = "any";
        any anyRhsType = visit(ctx->expr(0));
        if (anyRhsType.has_value() && anyRhsType.type() == typeid(string))
            rhsType = any_cast<string>(anyRhsType);

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

    // method dispatch
    if (ctx->expr().size() == 1 && ctx->OBJECTID().size() == 1 && ctx->TYPEID().empty())
    {
        auto anyRhsType = visit(ctx->expr(0));
        if (!anyRhsType.has_value() || anyRhsType.type() != typeid(string))
            return std::any{std::string{"__ERROR"}};

        std::string rhsType = any_cast<string>(anyRhsType);
        std::string methodName = ctx->OBJECTID(0)->getText();

        std::string cls =
            rhsType == "SELF_TYPE" ? current_class : rhsType;

        while (cls != "Object")
        {
            if (methodReturnTypes.count(cls) &&
                methodReturnTypes[cls].count(methodName))
            {
                std::string returnT = methodReturnTypes[cls][methodName];
                if (returnT == "SELF_TYPE")
                {
                    return std::any{std::string{"SELF_TYPE"}};
                }
                else
                {
                    return std::any{returnT};
                }
            }

            if (!parent.count(cls))
                break;

            cls = parent.at(cls);
        }

        errors.push_back(
            "Method `" + methodName + "` not defined for type `" + current_class +
            "` in dynamic dispatch");

        return std::any{std::string{"__ERROR"}};
    }

    // static dispatch
    if (ctx->expr().size() == 1 && ctx->OBJECTID().size() == 1 && ctx->TYPEID().size() == 1)
    {

        string staticType = ctx->TYPEID(0)->getText();
        string methodName = ctx->OBJECTID(0)->getText();

        string cls = staticType;

        while (cls != "Object")
        {

            if (methodReturnTypes.count(cls) &&
                methodReturnTypes[cls].count(methodName))
            {
                std::string returnT = methodReturnTypes[cls][methodName];
                if (returnT == "SELF_TYPE")
                {
                    return std::any{std::string{"SELF_TYPE"}};
                }
                else
                {
                    return std::any{returnT};
                }
            }
            cls = parent.at(cls);
        }

        errors.push_back(
            "Method `" + methodName + "` not defined for type `" + staticType +
            "` in static dispatch");
        return std::any{std::string{"__ERROR"}};
    }

    if (!ctx->OBJECTID().empty() && ctx->OBJECTID(0)->getText() == "self")
    {
        return std::any{std::string{"SELF_TYPE"}};
    }

    if (!ctx->OBJECTID().empty())
    {
        std::string name = ctx->OBJECTID(0)->getText();

        if (name == "self")
            return std::any{std::string{"SELF_TYPE"}};

        if (attrTypes.count(name))
            return std::any{attrTypes[name]};

        errors.push_back(
            "Variable named `" + name + "` not in scope");

        return std::any{std::string{"__ERROR"}};
    }

    return visitChildren(ctx);
}

std::any TypeChecker::visitAttr(CoolParser::AttrContext *ctx)
{
    std::string attrName = ctx->OBJECTID()->getText();

    if (!attrTypes.count(attrName))
    {
        return nullptr;
    }

    std::string declaredType = attrTypes[attrName];

    if (ctx->expr())
    {
        auto initTypeAny = visit(ctx->expr());
        if (initTypeAny.has_value())
        {
            std::string initType = std::any_cast<std::string>(initTypeAny);
            if (initType != declaredType && initType != "SELF_TYPE" && initType != "__ERROR")
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
