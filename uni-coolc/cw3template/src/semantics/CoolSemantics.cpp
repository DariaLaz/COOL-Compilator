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
    unordered_map<string, string> &parent);

vector<vector<string>> detectInheritanceLoops(
    const unordered_map<string, string> &parent);
// Runs semantic analysis and returns a list of errors, if any.
//
// TODO: change the type from void * to your typed AST type
expected<void *, vector<string>> CoolSemantics::run()
{
    vector<string> errors;

    // collect classes
    unordered_map<string, CoolParser::ClassContext *> classes;
    unordered_map<string, string> parent;

    // build inheritance graph
    auto *program = parser_->program();
    collectClasses(program, classes, parent);
    parser_->reset();

    // check inheritance graph is a tree

    // collect features

    // check methods are overridden correctly

    auto loops = detectInheritanceLoops(parent);
    if (!loops.empty())
    {
        errors.push_back(print_inheritance_loops_error(loops));
    }

    for (const auto &error : TypeChecker().check(parser_))
    {
        errors.push_back(error);
    }

    if (!errors.empty())
    {
        return unexpected(errors);
    }

    // return the typed AST
    return nullptr;
}

// utils

void collectClasses(CoolParser::ProgramContext *program, unordered_map<string, CoolParser::ClassContext *> &classes,
                    unordered_map<string, string> &parent)
{

    for (auto *cls : program->class_())
    {
        string className = cls->TYPEID(0)->getText();

        // if (classes.count(className))
        // {
        //     errors.push_back("Class " + className + " is defined multiple times.");
        //     return unexpected(errors);
        // }

        classes[className] = cls;

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

// inheritance_loops_error

vector<vector<string>> detectInheritanceLoops(
    const unordered_map<string, string> &parent)
{
    vector<vector<string>> loops;
    unordered_set<string> globallyVisited;

    for (const auto &[cls, _] : parent)
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
                vector<string> loop;
                for (int i = indexInPath[current]; i < path.size(); ++i)
                {
                    loop.push_back(path[i]);
                }

                reverse(loop.begin(), loop.end());
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
