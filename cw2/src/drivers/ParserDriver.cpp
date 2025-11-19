#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "antlr4-runtime/antlr4-runtime.h"

#include "CoolLexer.h"
#include "CoolParser.h"
#include "CoolParserBaseVisitor.h"

using namespace std;
using namespace antlr4;

namespace fs = filesystem;

/// Converts token to coursework-specific string representation.
string cool_token_to_string(CoolLexer *lexer, Token *token)
{
    auto token_type = token->getType();

    // clang-format off
    switch (token_type) {
        case static_cast<size_t>(-1) : return "EOF";

        case CoolLexer::SEMI   : return "';'";
        case CoolLexer::OCURLY : return "'{'";
        case CoolLexer::CCURLY : return "'}'";
        case CoolLexer::OPAREN : return "'('";
        case CoolLexer::COMMA  : return "','";
        case CoolLexer::CPAREN : return "')'";
        case CoolLexer::COLON  : return "':'";
        case CoolLexer::AT     : return "'@'";
        case CoolLexer::DOT    : return "'.'";
        case CoolLexer::PLUS   : return "'+'";
        case CoolLexer::MINUS  : return "'-'";
        case CoolLexer::STAR   : return "'*'";
        case CoolLexer::SLASH  : return "'/'";
        case CoolLexer::TILDE  : return "'~'";
        case CoolLexer::LT     : return "'<'";
        case CoolLexer::EQ     : return "'='";

        case CoolLexer::DARROW : return "DARROW";
        case CoolLexer::ASSIGN : return "ASSIGN";
        case CoolLexer::LE     : return "LE";

        case CoolLexer::CLASS  : return "CLASS";
        case CoolLexer::ELSE   : return "ELSE";
        case CoolLexer::FI     : return "FI";
        case CoolLexer::IF     : return "IF";
        case CoolLexer::IN     : return "IN";
        case CoolLexer::INHERITS : return "INHERITS";
        case CoolLexer::ISVOID : return "ISVOID";
        case CoolLexer::LET    : return "LET";
        case CoolLexer::LOOP   : return "LOOP";
        case CoolLexer::POOL   : return "POOL";
        case CoolLexer::THEN   : return "THEN";
        case CoolLexer::WHILE  : return "WHILE";
        case CoolLexer::CASE   : return "CASE";
        case CoolLexer::ESAC   : return "ESAC";
        case CoolLexer::NEW    : return "NEW";
        case CoolLexer::OF     : return "OF";
        case CoolLexer::NOT    : return "NOT";

        case CoolLexer::BOOL_CONST : return "BOOL_CONST";
        case CoolLexer::INT_CONST  : return "INT_CONST = " + token->getText();
        case CoolLexer::STR_CONST  : return "STR_CONST";
        case CoolLexer::TYPEID     : return "TYPEID = " + token->getText();
        case CoolLexer::OBJECTID   : return "OBJECTID = " + token->getText();
        case CoolLexer::ERROR      : return "ERROR";

        default : return "<Invalid Token>: " + token->toString();
    }
    // clang-format on
}

class TreePrinter : public CoolParserBaseVisitor
{
private:
    CoolLexer *lexer_;
    CoolParser *parser_;
    string file_name_;
    int indent = 0;
    bool debug = false;

    void increaseIndent()
    {
        this->indent += 2;
    }

    void decreaseIndent()
    {
        this->indent -= 2;
    }

    void printLine(string str)
    {
        if (debug)
        {
            cout << ">";
        }
        for (int i = 0; i < indent; i++)
        {
            cout << " ";
        }

        cout << str << endl;
    }

    void printRow(size_t row)
    {
        printLine("#" + std::to_string(row));
    }

public:
    TreePrinter(CoolLexer *lexer, CoolParser *parser, const string &file_name)
        : lexer_(lexer), parser_(parser), file_name_(file_name) {}

    any visitProgram(CoolParser::ProgramContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_program");
        this->increaseIndent();
        visitChildren(ctx);
        this->decreaseIndent();

        return any{};
    }

    any visitClass(CoolParser::ClassContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_class");
        this->increaseIndent();
        printLine(ctx->TYPEID()->getText());
        printLine("Object");
        printLine("\"" + file_name_ + '"');
        printLine("(");
        visitChildren(ctx);
        printLine(")");
        this->decreaseIndent();

        return any{};
    }

    any visitFeature(CoolParser::FeatureContext *ctx) override
    {
        visitChildren(ctx);
        return any{};
    }

    any visitAttribute(CoolParser::AttributeContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_attr");
        this->increaseIndent();
        printLine(ctx->OBJECTID()->getText());
        printLine(ctx->TYPEID()->getText());

        // todo move
        printRow(ctx->getStop()->getLine());
        printLine("_no_expr");
        printLine(": _no_type");

        this->decreaseIndent();

        return any{};
    }

    any visitMethod(CoolParser::MethodContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_method");
        this->increaseIndent();
        printLine(ctx->OBJECTID()->getText());

        for (auto f : ctx->formal())
        {
            visit(f);
        }

        printLine(ctx->TYPEID()->getText());

        visit(ctx->expresion());

        this->decreaseIndent();

        return any{};
    }

    any visitFormal(CoolParser::FormalContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_formal");
        this->increaseIndent();
        printLine(ctx->OBJECTID()->getText());
        printLine(ctx->TYPEID()->getText());
        this->decreaseIndent();

        return any{};
    }

    any visitExpresion(CoolParser::ExpresionContext *ctx) override
    {
        visitChildren(ctx);
        return any{};
    }

    any visitObject(CoolParser::ObjectContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_object");
        this->increaseIndent();
        printLine(ctx->OBJECTID()->getText());
        this->decreaseIndent();
        printLine(": _no_type");

        return any{};
    }

    any visitAssign(CoolParser::AssignContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_assign");
        this->increaseIndent();
        printLine(ctx->OBJECTID()->getText());
        visit(ctx->object());
        this->decreaseIndent();
        printLine(": _no_type");

        return any{};
    }

    any visitStaticDispatch(CoolParser::StaticDispatchContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_static_dispatch");
        this->increaseIndent();
        visit(ctx->object());
        printLine(ctx->TYPEID()->getText());
        printLine(ctx->OBJECTID()->getText());

        printLine("(");
        for (auto a : ctx->argument())
        {
            visit(a);
        }
        printLine(")");

        this->decreaseIndent();
        printLine(": _no_type");

        return any{};
    }

    any visitDispatch(CoolParser::DispatchContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_dispatch");
        this->increaseIndent();
        // self
        printRow(ctx->getStop()->getLine());
        printLine("_object");
        this->increaseIndent();
        printLine("self");
        this->decreaseIndent();
        printLine(": _no_type");

        // fn
        printLine(ctx->OBJECTID()->getText());

        printLine("(");
        for (auto a : ctx->argument())
        {
            visit(a);
        }
        printLine(")");

        this->decreaseIndent();
        printLine(": _no_type");

        return any{};
    }

    any visitCondition(CoolParser::ConditionContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_cond");
        this->increaseIndent();
        for (auto o : ctx->object())
        {
            visit(o);
        }
        this->decreaseIndent();
        printLine(": _no_type");

        return any{};
    }

    any visitWhile(CoolParser::WhileContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_loop");
        this->increaseIndent();
        for (auto o : ctx->object())
        {
            visit(o);
        }
        this->decreaseIndent();
        printLine(": _no_type");

        return any{};
    }

    any visitBlock(CoolParser::BlockContext *ctx) override
    {
        printRow(ctx->getStop()->getLine());
        printLine("_block");
        this->increaseIndent();
        for (auto e : ctx->expresion())
        {
            visit(e);
        }
        this->decreaseIndent();
        printLine(": _no_type");

        return any{};
    }

    any visitLetIn(CoolParser::LetInContext *ctx) override
    {
        int argsCount = ctx->letInArg().size();

        for (auto arg : ctx->letInArg())
        {
            printRow(ctx->getStop()->getLine());
            printLine("_let");
            this->increaseIndent();
            printLine(arg->OBJECTID()->getText());
            printLine(arg->TYPEID()->getText());
            // todo
            printRow(arg->getStop()->getLine());
            printLine("_no_expr");
            printLine(": _no_type");
        }

        visit(ctx->object());

        for (int i = 0; i < argsCount; i++)
        {

            this->decreaseIndent();
            printLine(": _no_type");
        }

        return any{};
    }

public:
    void print() { visitProgram(parser_->program()); }
};

class ErrorPrinter : public BaseErrorListener
{
private:
    string file_name_;
    CoolLexer *lexer_;
    bool has_error_ = false;

public:
    ErrorPrinter(const string &file_name, CoolLexer *lexer)
        : file_name_(file_name), lexer_(lexer) {}

    virtual void syntaxError(Recognizer *recognizer, Token *offendingSymbol,
                             size_t line, size_t charPositionInLine,
                             const std::string &msg,
                             std::exception_ptr e) override
    {
        has_error_ = true;
        cout << '"' << file_name_ << "\", line " << line
             << ": syntax error at or near "
             << cool_token_to_string(lexer_, offendingSymbol) << endl;
    }

    bool has_error() const { return has_error_; }
};

int main(int argc, const char *argv[])
{
    if (argc != 2)
    {
        cerr << "Expecting exactly one argument: name of input file" << endl;
        return 1;
    }

    auto file_path = argv[1];
    ifstream fin(file_path);

    auto file_name = fs::path(file_path).filename().string();

    ANTLRInputStream input(fin);
    CoolLexer lexer(&input);

    CommonTokenStream tokenStream(&lexer);

    CoolParser parser(&tokenStream);

    ErrorPrinter error_printer(file_name, &lexer);

    parser.removeErrorListener(&ConsoleErrorListener::INSTANCE);
    parser.addErrorListener(&error_printer);

    // This will trigger the error_printer, in case there are errors.
    parser.program();
    parser.reset();

    if (!error_printer.has_error())
    {
        TreePrinter(&lexer, &parser, file_name).print();
    }
    else
    {
        cout << "Compilation halted due to lex and parse errors" << endl;
    }

    return 0;
}
