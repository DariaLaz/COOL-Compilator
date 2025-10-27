#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "antlr4-runtime/antlr4-runtime.h"

#include "CoolLexer.h"

using namespace std;
using namespace antlr4;

/// Преобразува жетон в текст, който очаква системата за проверка на курсовата работа (част 1).
string cool_token_to_string(Token *token)
{
    auto token_type = token->getType();

    switch (token_type)
    {
    case static_cast<size_t>(-1):
        return "EOF";

    case CoolLexer::SEMI:
        return "';'";
    case CoolLexer::DOT:
        return "'.'";
    case CoolLexer::DASH:
        return "'-'";
        // Добавете тук останалите жетони, които представляват само един символ.

    case CoolLexer::DARROW:
        return "DARROW";
    case CoolLexer::BOOL_CONST:
        return "BOOL_CONST";
    case CoolLexer::STR_CONST:
        return "STR_CONST";
    case CoolLexer::CLASS:
        return "CLASS";
    case CoolLexer::ELSE:
        return "ELSE";
    case CoolLexer::FI:
        return "FI";
    case CoolLexer::IF:
        return "IF";
    case CoolLexer::IN:
        return "IN";
    case CoolLexer::INHERITS:
        return "INHERITS";
    case CoolLexer::ISVOID:
        return "ISVOID";
    case CoolLexer::LET:
        return "LET";
    case CoolLexer::LOOP:
        return "LOOP";
    case CoolLexer::POOL:
        return "POOL";
    case CoolLexer::THEN:
        return "THEN";
    case CoolLexer::WHILE:
        return "WHILE";
    case CoolLexer::CASE:
        return "CASE";
    case CoolLexer::ESAC:
        return "ESAC";
    case CoolLexer::NEW:
        return "NEW";
    case CoolLexer::OF:
        return "OF";
    case CoolLexer::NOT:
        return "NOT";
    case CoolLexer::INT_CONST:
        return "INT_CONST";
    case CoolLexer::TYPEID:
        return "TYPEID";
    case CoolLexer::OBJECTID:
        return "OBJECTID";
    case CoolLexer::ERROR:
        return "ERROR:";

    default:
        return "<Invalid Token>: " + token->toString();
    }
}

void dump_cool_token(CoolLexer *lexer, ostream &out, Token *token)
{
    if (token->getType() == static_cast<size_t>(-1))
    {
        // Жетонът е EOF, така че не го принтирам.
        return;
    }

    out << "#" << token->getLine() << " " << cool_token_to_string(token);

    auto token_type = token->getType();
    auto token_start_char_index = token->getStartIndex();
    switch (token_type) {
    case CoolLexer::BOOL_CONST: {
        out << " "
            << (lexer->get_bool_value(token_start_char_index) ? "true"
                                                              : "false");
        break;
    }

    case CoolLexer::STR_CONST: {
        out << " " << '"' << lexer->get_string_value(token_start_char_index) << '"';
        break;
    }

    case CoolLexer::INT_CONST:
    {
        out << " " << lexer->get_string_value(token_start_char_index);
        break;
    }
    
    case CoolLexer::TYPEID:
    case CoolLexer::OBJECTID:
    {
        out << " " << lexer->get_id_value(token_start_char_index);
        break;
    }

    case CoolLexer::ERROR: {
        out << " " << lexer->get_error_message(token_start_char_index);
        break;
    }
    }

    out << endl;
}

int main(int argc, const char *argv[])
{
    ANTLRInputStream input(cin);
    CoolLexer lexer(&input);

    // За временно скриване на грешките:
    // lexer.removeErrorListener(&ConsoleErrorListener::INSTANCE);

    CommonTokenStream tokenStream(&lexer);

    tokenStream.fill(); // Изчитане на всички жетони.

    vector<Token *> tokens = tokenStream.getTokens();
    for (Token *token : tokens)
    {
        dump_cool_token(&lexer, cout, token);
    };

    return 0;
}
