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
    pushScope();
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
        {
            params.push_back(f->TYPEID()->getText());
        }

        methodParamTypes[current_class][name] = params;
    }

    auto c = visitChildren(ctx);
    popScope();

    return c;
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
        scopes.back()[attrName] = declaredType;
    }
}

std::any TypeChecker::visitMethod(CoolParser::MethodContext *ctx)
{
    string methodName = ctx->OBJECTID()->getText();
    string declaredReturnType = ctx->TYPEID()->getText();

    pushScope();
    for (auto *f : ctx->formal())
        scopes.back()[f->OBJECTID()->getText()] = f->TYPEID()->getText();

    current_method = methodName;

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
    else if (bodyType == "SELF_TYPE" && !visitedMethods.count(methodName))
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
    else if (bodyType != "__ERROR" && bodyType != "SELF_TYPE")
    {
        bool isSubtype = false;
        string t = bodyType;
        while (true)
        {
            if (t == declaredReturnType)
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
                "In class `" + current_class +
                "` method `" + methodName +
                "`: `" + bodyType +
                "` is not `" + declaredReturnType +
                "`: type of method body is not a subtype of return type");
        }
    }

    visitedMethods.insert(methodName);

    popScope();
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

        std::string rhsType = "any";
        any anyRhsType = visit(ctx->expr(0));
        if (anyRhsType.has_value() && anyRhsType.type() == typeid(string))
            rhsType = any_cast<string>(anyRhsType);

        if (!attrTypes.count(lhsName))
        {
            errors.push_back(
                "Assignee named `" + lhsName + "` not in scope");

            return rhsType;
        }

        std::string lhsType = attrTypes[lhsName];

        unordered_set<string> possibleSelfTypes;
        unordered_set<string> visitedP;

        possibleSelfTypes.insert(lhsType);
        possibleSelfTypes.insert("Object");

        if (parent.count(lhsType))
        {

            string current = parent.at(lhsType);
            while (classes.count(current))
            {
                if (visitedP.count(current))
                {
                    break;
                }
                visitedP.insert(current);

                possibleSelfTypes.insert(current);
                if (!parent.count(current))
                {
                    break;
                }
                current = parent.at(current);
            }
        }

        if (!possibleSelfTypes.count(rhsType))
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

    if (ctx->IF())
    {
        auto condAny = visit(ctx->expr(0));
        if (!condAny.has_value() || condAny.type() != typeid(string))
            return std::any{std::string{"__ERROR"}};

        string condType = any_cast<string>(condAny);
        if (condType != "Bool")
        {
            errors.push_back(
                "Type `" + condType +
                "` of if-then-else-fi condition is not `Bool`");
        }

        auto thenAny = visit(ctx->expr(1));
        auto elseAny = visit(ctx->expr(2));

        if (!thenAny.has_value() || !elseAny.has_value())
            return std::any{std::string{"__ERROR"}};

        string t1 = any_cast<string>(thenAny);
        string t2 = any_cast<string>(elseAny);

        if (t1 == t2)
            return std::any{t1};

        // find he common type
        unordered_set<string> seen;
        string a = t1;
        while (true)
        {
            seen.insert(a);
            if (!parent.count(a))
                break;
            a = parent.at(a);
        }

        string b = t2;
        while (!seen.count(b))
        {
            if (!parent.count(b))
                break;
            b = parent.at(b);
        }

        return any{b};
    }

    if (ctx->WHILE())
    {
        auto a = visit(ctx->expr(0));
        auto cond = a.has_value() && a.type() == typeid(string) ? any_cast<string>(a) : "__ERROR";

        bool isSubtype = false;
        auto t = cond;
        while (true)
        {
            if (t == "Bool")
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
                "Type `" + cond +
                "` of while-loop-pool condition is not `Bool`");
        }

        visit(ctx->expr(1));
        return any{string{"Object"}};
    }

    if (ctx->LET())
    {
        pushScope();
        for (auto *v : ctx->vardecl())
        {

            string name = v->OBJECTID()->getText();
            string type = v->TYPEID()->getText();
            if (v->expr())
            {
                auto exprTypeAny = (visit(v->expr()));
                string exprType = "__ERROR";
                if (exprTypeAny.has_value() && exprTypeAny.type() == typeid(std::string))
                {
                    exprType = any_cast<string>(exprTypeAny);
                }

                bool isSubtype = false;
                auto t = exprType;
                while (true)
                {
                    if (t == type)
                    {
                        isSubtype = true;
                        break;
                    }
                    if (!parent.count(t))
                        break;
                    t = parent.at(t);
                }

                if (!isSubtype && type != "__ERROR" && exprType != "__ERROR")
                {
                    errors.push_back("Initializer for variable `" + name + "` in let-in expression is of type `" + exprType + "` which is not a subtype of the declared type `" + type + "`");
                }
            }

            scopes.back()[name] = type;
        }
        auto r = visit(ctx->expr().back());
        popScope();

        return r;
    }

    // case
    if (ctx->CASE())
    {
        unordered_set<string> branchTypes;
        unordered_set<string> declaredBranchTypes;

        visit(ctx->expr(0));

        for (size_t i = 0; i < ctx->OBJECTID().size(); ++i)
        {
            string varName = ctx->OBJECTID(i)->getText();
            string varType = ctx->TYPEID(i)->getText();
            pushScope();
            if (!classes.count(varType) &&
                varType != "Int" && varType != "Bool" &&
                varType != "String" && varType != "Object")
            {
                errors.push_back(
                    "Option `" + varName + "` in case-of-esac declared to have unknown type `" + varType + "`");
            }
            else
            {

                scopes.back()[varName] = varType;
            }

            if (declaredBranchTypes.count(varType))
            {
                errors.push_back("Multiple options match on type `" + varType + "`");
            }
            declaredBranchTypes.insert(varType);

            auto bodyAny = visit(ctx->expr(i + 1));
            if (bodyAny.has_value() && bodyAny.type() == typeid(string))
            {
                auto a = any_cast<string>(bodyAny);
                branchTypes.insert(a);
            }

            popScope();
        }

        if (branchTypes.empty())
            return any{string{"__ERROR"}};

        string lub = *branchTypes.begin();
        for (auto &cur : branchTypes)
        {
            unordered_set<string> ancestors;

            string t = lub;
            while (true)
            {
                ancestors.insert(t);
                if (!parent.count(t))
                    break;
                t = parent.at(t);
            }

            t = cur;
            lub = "__ERROR";
            while (true)
            {
                if (ancestors.count(t))
                {
                    lub = t;
                    break;
                }
                if (!parent.count(t))
                    break;
                t = parent.at(t);
            }
        }
        return any{lub};
    }

    // implicit dispatch
    if (ctx->OBJECTID().size() == 1 &&
        ctx->TYPEID().empty() &&
        (ctx->DOT() == nullptr && ctx->AT() == nullptr) &&
        ctx->OPAREN() != nullptr)
    {

        std::string methodName = ctx->OBJECTID(0)->getText();
        std::vector<std::string> argTypes;
        argTypes.reserve(ctx->expr().size());
        for (size_t i = 0; i < ctx->expr().size(); ++i)
        {
            auto a = visit(ctx->expr(i));
            if (a.has_value() && a.type() == typeid(std::string))
            {
                argTypes.push_back(std::any_cast<std::string>(a));
            }
            else
                argTypes.push_back("__ERROR");
        }

        std::string cls = current_class;

        while (true)
        {
            if (methodReturnTypes.count(cls) && methodReturnTypes[cls].count(methodName))
            {
                auto &params = methodParamTypes[cls][methodName];

                if (params.size() != argTypes.size())
                {
                    errors.push_back(
                        "Method `" + methodName + "` of type `" + cls +
                        "` called with the wrong number of arguments; " +
                        std::to_string(params.size()) +
                        " arguments expected, but " +
                        std::to_string(argTypes.size()) + " provided");
                    return std::any{std::string{"__ERROR"}};
                }

                for (size_t i = 0; i < params.size(); ++i)
                {
                    std::string actual = argTypes[i];
                    std::string expected = params[i];

                    std::string t = (actual == "SELF_TYPE") ? current_class : actual;

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
                            "` from class `" + current_class + "`:");
                        errors.push_back(
                            "  `" + actual + "` is not a subtype of `" + expected +
                            "`: argument at position " + std::to_string(i) +
                            " (0-indexed) has the wrong type");
                    }
                }

                std::string ret = methodReturnTypes[cls][methodName];
                return (ret == "SELF_TYPE")
                           ? std::any{std::string{"SELF_TYPE"}}
                           : std::any{ret};
            }

            if (!parent.count(cls))
                break;
            cls = parent.at(cls);
        }
    }

    // method dispatch
    if (ctx->expr().size() >= 1 &&
        ctx->OBJECTID().size() == 1 &&
        ctx->TYPEID().empty() && ctx->DOT() != nullptr)
    {
        auto anyRhsType = visit(ctx->expr(0));
        if (!anyRhsType.has_value() || anyRhsType.type() != typeid(string))
            return std::any{std::string{"__ERROR"}};

        std::string rhsType = any_cast<string>(anyRhsType);
        std::string methodName = ctx->OBJECTID(0)->getText();

        std::string cls =
            rhsType == "SELF_TYPE" ? current_class : rhsType;

        vector<string> argTypes;
        for (size_t i = 1; i < ctx->expr().size(); ++i)
        {
            auto a = visit(ctx->expr(i));
            if (a.has_value() && a.type() == typeid(string))
                argTypes.push_back(any_cast<string>(a));
            else
                argTypes.push_back("__ERROR");
        }

        while (cls != "Object")
        {
            if (methodReturnTypes.count(cls) &&
                methodReturnTypes[cls].count(methodName))
            {
                auto &params = methodParamTypes[cls][methodName];

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
                    string actual = argTypes[i];
                    string expected = params[i];

                    string t = actual == "SELF_TYPE"
                                   ? current_class
                                   : actual;

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

                        string base = (rhsType == "SELF_TYPE") ? current_class : rhsType;

                        errors.push_back(
                            "Invalid call to method `" + methodName +
                            "` from class `" + base + "`:");
                        errors.push_back(
                            "  `" + actual + "` is not a subtype of `" +
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
                break;

            cls = parent.at(cls);
        }

        string base = (rhsType == "SELF_TYPE") ? current_class : rhsType;
        if (base != "__ERROR")
            errors.push_back(
                "Method `" + methodName +
                "` not defined for type `" + base +
                "` in dynamic dispatch");

        return std::any{std::string{"__ERROR"}};
    }

    // static dispatch
    if (ctx->expr().size() >= 1 && ctx->OBJECTID().size() >= 1 && ctx->TYPEID().size() == 1 && ctx->AT() != nullptr)
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
                            "Invalid call to method `" + methodName + "` from class `" + cls + "`:");
                        errors.push_back("  `" +
                                         actualArg + "` is not a subtype of `" + expected +
                                         "`: argument at position " + to_string(i) +
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

        string t;
        if (lookVarInAllScopes(name, t))
            return any{t};

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

            unordered_set<string> possibleSelfTypes;
            unordered_set<string> visitedP;

            possibleSelfTypes.insert(initType);
            possibleSelfTypes.insert("Object");

            if (parent.count(initType))
            {
                string current = parent.at(initType);
                while (classes.count(current))
                {
                    if (visitedP.count(current))
                    {
                        break;
                    }
                    visitedP.insert(current);

                    possibleSelfTypes.insert(current);
                    if (!parent.count(current))
                    {
                        break;
                    }
                    current = parent.at(current);
                }
            }

            if (!possibleSelfTypes.count(declaredType) && initType != "SELF_TYPE" && initType != "__ERROR")
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

void TypeChecker::pushScope() { scopes.push_back({}); }
void TypeChecker::popScope() { scopes.pop_back(); }

bool TypeChecker::lookVarInAllScopes(string &name, string &out)
{
    for (int i = scopes.size() - 1; i >= 0; --i)
    {
        if (scopes[i].count(name))
        {
            out = scopes[i][name];
            return true;
        }
    }
    return false;
}