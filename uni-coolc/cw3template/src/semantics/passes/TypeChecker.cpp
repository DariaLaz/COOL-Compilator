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
    methodParamTypes[current_class].clear();
    for (auto *m : ctx->method())
    {
        string name = m->OBJECTID()->getText();
        string returnT = m->TYPEID()->getText();
        methodReturnTypes[current_class][name] = returnT;

        vector<string> params;
        for (auto *f : m->formal())
            params.push_back(f->TYPEID()->getText());

        methodParamTypes[current_class][name] = params;
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
    if (ctx->expr().size() >= 1 && ctx->OBJECTID().size() >= 1 && ctx->TYPEID().empty())
    {
        auto anyRhsType = visit(ctx->expr(0));
        if (!anyRhsType.has_value() || anyRhsType.type() != typeid(string))
            return std::any{std::string{"__ERROR"}};

        std::string rhsType = any_cast<string>(anyRhsType);
        std::string methodName = ctx->OBJECTID(0)->getText();

        std::string cls =
            rhsType == "SELF_TYPE" ? current_class : rhsType;

        vector<string> argTypes;
        if (ctx->expr(1))
            for (auto *e : ctx->expr(1)->expr())
                argTypes.push_back(any_cast<string>(visit(e)));

        while (cls != "Object")
        {
            if (methodReturnTypes.count(cls) &&
                methodReturnTypes[cls].count(methodName))
            {
                if (methodParamTypes.count(cls) &&
                    methodParamTypes[cls].count(methodName))
                {
                    auto &params = methodParamTypes[cls][methodName];
                    if (params.size() != argTypes.size())
                    {
                        errors.push_back(
                            "Method `" + methodName + "` of type `" + cls +
                            "` called with the wrong number of arguments; " +
                            to_string(params.size()) + " arguments expected, but " +
                            to_string(argTypes.size()) + " provided");
                        return std::any{std::string{"__ERROR"}};
                    }

                    for (size_t i = 0; i < params.size(); ++i)
                    {
                        auto argAny = visit(ctx->expr(i + 1));
                        if (!argAny.has_value() || argAny.type() != typeid(string))
                            continue;

                        string actualArgType = any_cast<string>(argAny);
                        string expectedType = params[i];

                        string t = actualArgType == "SELF_TYPE" ? current_class : actualArgType;
                        bool isSubtype = false;

                        while (true)
                        {
                            if (t == expectedType)
                            {
                                isSubtype = true;
                                break;
                            }
                            if (!parent.count(t))
                                break;
                            t = parent.at(t);
                        }

                        if (!isSubtype)
                        {
                            errors.push_back(
                                "Invalid call to method `" + methodName + "` from class `" + cls + "`:\n  `" +
                                actualArgType + "` is not a subtype of `" + expectedType +
                                "`: argument at position " + to_string(i) +
                                " (0-indexed) has the wrong type");
                        }
                    }
                }

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

        std::string initCls =
            rhsType == "SELF_TYPE" ? current_class : rhsType;

        if (initCls != "__ERROR")
            errors.push_back(
                "Method `" + methodName + "` not defined for type `" + initCls +
                "` in dynamic dispatch");

        return std::any{std::string{"__ERROR"}};
    }

    // static dispatch
    if (ctx->expr().size() >= 1 && ctx->OBJECTID().size() >= 1 && ctx->TYPEID().size() == 1)
    {

        auto anyRhsType = visit(ctx->expr(0));
        if (!anyRhsType.has_value() || anyRhsType.type() != typeid(string))
            return std::any{std::string{"__ERROR"}};
        std::string rhsType = any_cast<string>(anyRhsType);
        std::string actualCls =
            rhsType == "SELF_TYPE" ? current_class : rhsType;

        string staticType = ctx->TYPEID(0)->getText();
        string methodName = ctx->OBJECTID(0)->getText();

        if (staticType != "Int" && staticType != "Bool" && staticType != "String" && staticType != "Object" && !classes.count(staticType))
        {
            errors.push_back(
                "Undefined type `" + staticType + "` in static method dispatch");

            if (!methodReturnTypes[actualCls].count(methodName))
                errors.push_back(
                    "Method `" + methodName + "` not defined for type `" +
                    actualCls + "` in static dispatch");

            return std::any{std::string{"__ERROR"}};
        }

        bool isSubtype = false;
        std::string t = actualCls;
        while (true)
        {
            if (t == staticType)
            {
                isSubtype = true;
                break;
            }
            if (!parent.count(t))
                break;
            t = parent.at(t);
        }

        if (!isSubtype)
        {
            errors.push_back(
                "`" + actualCls + "` is not a subtype of `" + staticType + "`");
        }

        string cls = staticType;

        while (cls != "Object")
        {

            if (methodReturnTypes.count(cls) &&
                methodReturnTypes[cls].count(methodName))
            {

                auto &params = methodParamTypes[cls][methodName];

                vector<string> argTypes;
                for (size_t i = 1; i < ctx->expr().size(); ++i)
                {
                    auto a = visit(ctx->expr(i));
                    if (a.has_value() && a.type() == typeid(string))
                        argTypes.push_back(any_cast<string>(a));
                }

                if (params.size() != argTypes.size())
                {
                    errors.push_back(
                        "Method `" + methodName + "` of type `" + cls +
                        "` called with the wrong number of arguments; " +
                        to_string(params.size()) +
                        " arguments expected, but " +
                        to_string(argTypes.size()) + " provided");
                    return std::any{std::string{"__ERROR"}};
                }

                for (size_t i = 0; i < params.size(); ++i)
                {
                    string actualArg = argTypes[i];
                    string expected = params[i];

                    string t = actualArg == "SELF_TYPE"
                                   ? current_class
                                   : actualArg;

                    bool isSubtype = false;
                    while (true)
                    {
                        if (t == expected)
                        {
                            isSubtype = true;
                            break;
                        }
                        if (!parent.count(t))
                            break;
                        t = parent.at(t);
                    }

                    if (!isSubtype)
                    {
                        errors.push_back(
                            "Invalid call to method `" + methodName +
                            "` from class `" + cls + "`:\n  `" +
                            actualArg + "` is not a subtype of `" +
                            expected +
                            "`: argument at position " +
                            to_string(i) +
                            " (0-indexed) has the wrong type");
                    }
                }

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
            {
                break;
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
