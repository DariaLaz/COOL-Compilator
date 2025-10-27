lexer grammar CoolLexer;

// Тази част позволява дефинирането на допълнителен код, който да се запише в
// генерирания CoolLexer.h.
//
// Коментарите вътре са на английски, понеже ANTLR4 иначе ги омазва.
@lexer::members {

    // ----------------------- booleans -------------------------

    // A map from token ids to boolean values
    std::map<int, bool> bool_values;

    void assoc_bool_with_token(bool value) {
        bool_values[tokenStartCharIndex] = value;
    }

    bool get_bool_value(int token_start_char_index) {
        return bool_values.at(token_start_char_index);
    }

    // ----------------------- strings -------------------------

    // Maximum length of a constant string literal (CSL) excluding the implicit
    // null character that marks the end of the string.
    const unsigned MAX_STR_CONST = 1024;
    std::vector<char> string_buffer;


    void assoc_string_with_token(const std::string &value) {
        
    }

    std::string get_string_value(int token_start_char_index) {
        return std::string();
    }
}

// --------------- прости жетони -------------------

SEMI   : ';';
DARROW : '=>';

WS : [ \r\n\u000D]+ -> skip ;

// Добавете тук останалите жетони, които представляват просто текст.


// --------------- булеви константи -------------------

BOOL_CONST : 'true'  { assoc_bool_with_token(true); }
           | 'false' { assoc_bool_with_token(false); };

// --------------- коментари -------------------



// --------------- ключови думи -------------------



// --------------- текстови низове -------------------

STR_CONST: '"' '"';

// TODO:
// ASSIGN BOOL_CONST CASE CLASS DARROW ELSE ESAC FI IF IN
// INHERITS INT_CONST ISVOID LE LET LOOP NEW NOT OBJECTID OF
// POOL STR_CONST THEN TYPEID WHILE

