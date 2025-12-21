#include "CoolSemantics.h"

#include <expected>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#include "passes/TypeChecker.h"

using namespace std;

string print_inheritance_loops_error(vector<vector<string>> inheritance_loops);

void collectClasses(
    CoolParser::ProgramContext *program,
    unordered_map<string, CoolParser::ClassContext *> &classes,
    unordered_map<string, string> &parent,
    vector<string> &classesInOrder, vector<std::string> &errors);

vector<vector<string>> detectInheritanceLoops(
    const unordered_map<string, string> &parent, vector<string> &classesInOrder);

vector<pair<string, string>> detectInheritanceUndefinedClass(
    const unordered_map<string, string> &parent,
    unordered_map<string, CoolParser::ClassContext *> &classes,
    vector<string> &classesInOrder, vector<string> &errors);

void print_inheritance_undefined_error(vector<std::string> &errors, vector<pair<string, string>> &inheritance_undefined);

void detectDuplicateMethods(
    unordered_map<string, CoolParser::ClassContext *> &classes,
    vector<string> &classesInOrder,
    vector<string> &errors);

void detectMethodOverrideErrors(
    unordered_map<string, CoolParser::ClassContext *> &classes,
    unordered_map<string, string> &parent,
    vector<string> &classesInOrder,
    vector<string> &errors);

void detectMethodUndefinedArgsErrors(
    unordered_map<string, CoolParser::ClassContext *> &classes,
    vector<string> &classesInOrder,
    vector<string> &errors);

void detectAttrOverrideErrors(
    unordered_map<string, CoolParser::ClassContext *> &classes,
    unordered_map<string, string> &parent,
    vector<string> &classesInOrder,
    vector<string> &errors);

// Runs semantic analysis and returns a list of errors, if any.
//
// TODO: change the type from void * to your typed AST type
expected<void *, vector<string>> CoolSemantics::run()
{
    vector<string> errors;

    unordered_map<string, CoolParser::ClassContext *> classes;
    vector<string> classesInOrder;
    unordered_map<string, string> parent;

    auto *program = parser_->program();
    collectClasses(program, classes, parent, classesInOrder, errors);

    auto loops = detectInheritanceLoops(parent, classesInOrder);
    if (!loops.empty())
    {
        errors.push_back(print_inheritance_loops_error(loops));
    }

    auto undefinedClasses =
        detectInheritanceUndefinedClass(parent, classes, classesInOrder, errors);

    if (!undefinedClasses.empty())
    {
        print_inheritance_undefined_error(errors, undefinedClasses);
    }

    detectDuplicateMethods(classes, classesInOrder, errors);
    detectMethodOverrideErrors(classes, parent, classesInOrder, errors);
    detectMethodUndefinedArgsErrors(classes, classesInOrder, errors);

    detectAttrOverrideErrors(classes, parent, classesInOrder, errors);

    parser_->reset();
    for (const auto &error : TypeChecker(classes, parent, classesInOrder).check(parser_))
    {
        errors.push_back(error);
    }

    if (!errors.empty())
    {
        return unexpected(errors);
    }

    return nullptr;
}

// utils

void collectClasses(CoolParser::ProgramContext *program, unordered_map<string, CoolParser::ClassContext *> &classes,
                    unordered_map<string, string> &parent, vector<string> &classesInOrder, vector<std::string> &errors)
{

    for (auto *cls : program->class_())
    {
        string className = cls->TYPEID(0)->getText();

        bool isBuildIn = className == "Object" ||
                         className == "Bool" ||
                         className == "Int" ||
                         className == "IO" ||
                         className == "String";

        if (isBuildIn || classes.count(className))
        {
            errors.push_back("Type `" + className + "` already defined");
            continue;
        }

        classes[className] = cls;
        classesInOrder.push_back(className);

        if (cls->TYPEID().size() > 1)
        {
            string parentName = cls->TYPEID(1)->getText();
            parent[className] = parentName;
        }
        else
        {
            parent[className] = "Object";
        }
    }
}

// inheritance loops error
vector<vector<string>> detectInheritanceLoops(
    const unordered_map<string, string> &parent, vector<string> &classesInOrder)
{
    vector<vector<string>> loops;
    unordered_set<string> globallyVisited;
    unordered_set<string> loopsVisited;

    for (const auto cls : classesInOrder)
    {
        if (globallyVisited.count(cls))
        {
            continue;
        }

        unordered_map<string, int> indexInPath;
        vector<string> path;

        string current = cls;

        while (parent.count(current))
        {
            if (indexInPath.count(current))
            {
                bool isAlreadyIn = false;
                vector<string> loop;
                for (int i = indexInPath[current]; i < path.size(); ++i)
                {
                    if (loopsVisited.count(path[i]))
                    {
                        isAlreadyIn = true;
                        break;
                    }
                    loop.push_back(path[i]);
                    loopsVisited.insert(path[i]);
                }

                if (isAlreadyIn)
                {
                    break;
                }

                loops.push_back(loop);
                break;
            }

            indexInPath[current] = path.size();
            path.push_back(current);
            globallyVisited.insert(current);

            current = parent.at(current);
        }
    }

    return loops;
}

string print_inheritance_loops_error(vector<vector<string>> inheritance_loops)
{
    stringstream eout;
    eout << "Detected " << inheritance_loops.size()
         << " loops in the type hierarchy:" << endl;
    for (int i = 0; i < inheritance_loops.size(); ++i)
    {
        eout << i + 1 << ") ";
        auto &loop = inheritance_loops[i];
        for (string name : loop)
        {
            eout << name << " <- ";
        }
        eout << endl;
    }

    return eout.str();
}

// inheritance undefined class error
vector<pair<string, string>> detectInheritanceUndefinedClass(
    const unordered_map<string, string> &parent,
    unordered_map<string, CoolParser::ClassContext *> &classes,
    vector<string> &classesInOrder, vector<string> &errors)
{
    vector<pair<string, string>> undefinedClasses;
    unordered_set<string> globallyVisited;

    for (const auto cls : classesInOrder)
    {
        string current = cls;

        while (parent.count(current))
        {
            if (globallyVisited.count(current))
            {
                break;
            }
            auto p = parent.at(current);

            bool isBuildIn = p == "Object" ||
                             p == "Bool" ||
                             p == "Int" ||
                             p == "IO" ||
                             p == "String";

            if (!classes.count(p) && !isBuildIn)
            {
                undefinedClasses.push_back({current, p});
            }

            bool hasError = p == "Bool" ||
                            p == "Int" ||
                            p == "String";

            if (hasError)
            {
                errors.push_back("`" + cls + "` inherits from `" + p + "` which is an error");
            }
            globallyVisited.insert(current);
            current = p;
        }
    }

    return undefinedClasses;
}

void print_inheritance_undefined_error(vector<std::string> &errors, vector<pair<string, string>> &inheritance_undefined)
{
    for (int i = 0; i < inheritance_undefined.size(); ++i)
    {
        auto p = inheritance_undefined[i];
        errors.push_back(p.first + " inherits from undefined class " + p.second);
    }
}

// duplicated methods

void detectDuplicateMethods(
    unordered_map<string, CoolParser::ClassContext *> &classes,
    vector<string> &classesInOrder,
    vector<string> &errors)
{
    for (const auto &clsName : classesInOrder)
    {
        auto *cls = classes.at(clsName);
        unordered_set<string> seen;

        for (auto *method : cls->method())
        {
            string name = method->OBJECTID()->getText();
            if (seen.count(name))
            {
                errors.push_back(
                    "Method `" + name + "` already defined for class `" + clsName + "`");
            }
            seen.insert(name);
        }
    }
}

// override errors

void detectMethodOverrideErrors(
    unordered_map<string, CoolParser::ClassContext *> &classes,
    unordered_map<string, string> &parent,
    vector<string> &classesInOrder,
    vector<string> &errors)
{
    unordered_set<string> problemClasses;
    for (const auto &clsName : classesInOrder)
    {
        auto *cls = classes.at(clsName);

        string current = parent.at(clsName);
        vector<string> pars;
        unordered_set<string> visitedP;
        while (classes.count(current))
        {
            if (visitedP.count(current))
            {
                break;
            }
            visitedP.insert(current);

            pars.push_back(current);
            current = parent.at(current);
        }

        reverse(pars.begin(), pars.end());

        unordered_set<string> visitedMethods;
        for (auto *method : cls->method())
        {
            string methodName = method->OBJECTID()->getText();

            if (visitedMethods.count(methodName))
            {
                continue;
            }
            visitedMethods.insert(methodName);

            vector<string> argTypes;
            for (auto *formal : method->formal())
            {
                argTypes.push_back(formal->TYPEID()->getText());
            }
            string returnType = method->TYPEID()->getText();

            bool found = false;
            for (auto cp : pars)
            {
                if (problemClasses.count(cp))
                {
                    continue;
                }
                auto *p = classes.at(cp);

                for (auto *pm : p->method())
                {
                    if (pm->OBJECTID()->getText() == methodName)
                    {
                        vector<string> parentArgs;
                        for (auto *f : pm->formal())
                        {
                            parentArgs.push_back(f->TYPEID()->getText());
                        }
                        string parentReturn = pm->TYPEID()->getText();

                        if (argTypes != parentArgs || returnType != parentReturn)
                        {
                            errors.push_back(
                                "Override for method " + methodName +
                                " in class " + clsName +
                                " has different signature than method in ancestor " +
                                cp + " (earliest ancestor that mismatches)");
                            problemClasses.insert(clsName);
                            found = true;
                        }
                        break;
                    }
                }

                if (found)
                    break;
            }
        }
    }
}

void detectMethodUndefinedArgsErrors(
    unordered_map<string, CoolParser::ClassContext *> &classes,
    vector<string> &classesInOrder,
    vector<string> &errors)
{
    for (const auto &clsName : classesInOrder)
    {
        auto *cls = classes.at(clsName);

        for (auto *method : cls->method())
        {
            string methodName = method->OBJECTID()->getText();
            for (auto *formal : method->formal())
            {
                auto argType = formal->TYPEID()->getText();

                if (!classes.count(argType))
                {
                    errors.push_back("Method `" + methodName + "` in class `" + clsName + "` declared to have an argument of type `" + argType + "` which is undefined");
                    break;
                }
            }
        }
    }
}

void detectAttrOverrideErrors(
    unordered_map<string, CoolParser::ClassContext *> &classes,
    unordered_map<string, string> &parent,
    vector<string> &classesInOrder,
    vector<string> &errors)
{
    unordered_set<string> problemClasses;
    for (const auto &clsName : classesInOrder)
    {
        auto *cls = classes.at(clsName);

        string current = parent.at(clsName);
        vector<string> pars;
        unordered_set<string> visitedP;
        while (classes.count(current))
        {
            if (visitedP.count(current))
            {
                break;
            }
            visitedP.insert(current);

            pars.push_back(current);
            current = parent.at(current);
        }

        reverse(pars.begin(), pars.end());

        unordered_set<string> visitedAttr;
        for (auto *attr : cls->attr())
        {
            string attrName = attr->OBJECTID()->getText();
            if (visitedAttr.count(attrName))
            {
                continue;
            }
            visitedAttr.insert(attrName);

            bool found = false;
            for (auto cp : pars)
            {
                if (problemClasses.count(cp))
                {
                    continue;
                }
                auto *p = classes.at(cp);

                for (auto *pa : p->attr())
                {
                    if (pa->OBJECTID()->getText() == attrName)
                    {

                        errors.push_back(
                            "Attribute `" + attrName + "` in class `" + clsName + "` redefines attribute with the same name in ancestor `" + cp + "` (earliest ancestor that defines this attribute)");
                        problemClasses.insert(clsName);
                        found = true;

                        break;
                    }
                }

                if (found)
                    break;
            }
        }
    }
}