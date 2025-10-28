lexer grammar CoolLexer;

// Тази част позволява дефинирането на допълнителен код, който да се запише в
// генерирания CoolLexer.h.
//
// Коментарите вътре са на английски, понеже ANTLR4 иначе ги омазва.
@lexer::members {

    std::vector<char> string_buffer;
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
    std::map<int, std::string> string_values;

    void assoc_string_with_token(const std::string &value) {
        string_values[tokenStartCharIndex] = value;
    }

    std::string get_string_value(int token_start_char_index) {
        return string_values.at(token_start_char_index);
    }

    std::string maybe_escape_control_char(char c) {
        if (c < 0x20 || c == 0x7F) {
            char buf[10];
            sprintf(buf, "<0x%02x>", c);

            return std::string(buf);
        }

        return std::string(1, c);
    }

    // ----------------------- identifiers -------------------------
    std::map<int, std::string> id_values;
    void assoc_id_with_token(const std::string &value) {
        id_values[tokenStartCharIndex] = value;
    }

    std::string get_id_value(int token_start_char_index) {
        return id_values.at(token_start_char_index);
    }


    // -------------------- errors -----------------------

    std::map<int, std::string> error_messages;

    void set_error_message(const std::string &message) {
        setType(ERROR);
        error_messages[tokenStartCharIndex] = message;
    }

    std::string get_error_message(int token_start_char_index) {
        return error_messages.at(token_start_char_index);
    }
}

// --------------- специални жетони -------------------

tokens { ERROR }

WS : [ \t\r\n]+ -> skip ;

// --------------- прости жетони -------------------

SEMI   : ';';
LBRACE : '{';  
RBRACE : '}';
LPAREN : '(';
COMMA  : ',';
RPAREN : ')';
COLON  : ':';
AT     : '@';
DOT    : '.';
PLUS   : '+';
MINUS  : '-';
MULT   : '*';
DIV    : '/';
TILDE  : '~';
LT     : '<';
EQ     : '=';
LE     : '<=';
DARROW : '=>';
ASSIGN : '<-';


// --------------- ключови думи -------------------

CLASS options { caseInsensitive=true; }: 'class' ;
ELSE options { caseInsensitive=true; }: 'else' ;
FI options { caseInsensitive=true; }: 'fi' ;
IF options { caseInsensitive=true; }: 'if' ;
IN options { caseInsensitive=true; }: 'in' ;
INHERITS options { caseInsensitive=true; }: 'inherits' ;
ISVOID options { caseInsensitive=true; }: 'isvoid' ;
LET options { caseInsensitive=true; }: 'let' ;
LOOP options { caseInsensitive=true; }: 'loop' ;
POOL options { caseInsensitive=true; }: 'pool' ;
THEN options { caseInsensitive=true; }: 'then' ;
WHILE options { caseInsensitive=true; }: 'while' ;
CASE options { caseInsensitive=true; }: 'case' ;
ESAC options { caseInsensitive=true; }: 'esac' ;
NEW options { caseInsensitive=true; }: 'new' ;
OF options { caseInsensitive=true; }: 'of' ;
NOT options { caseInsensitive=true; }: 'not' ;


// --------------- коментари -------------------

ONE_LINE_COMMENT : '--' ~[\n]* -> skip ;

// TODO: too slow for testing uncomment when needed
// Test 30 is not working it is trowing errors
// MULTILINE_COMMENT : '(*' ( MULTILINE_COMMENT | . )*? '*)' -> skip ;
MULTILINE_COMMENT : '(*' (.)*? '*)' -> skip ;


// --------------- булеви константи -------------------

BOOL_CONST : TRUE  { assoc_bool_with_token(true); }
           | FALSE { assoc_bool_with_token(false); };

fragment TRUE  : 't'('r'|'R')('u'|'U')('e'|'E');
fragment FALSE : 'f'('a'|'A')('l'|'L')('s'|'S')('e'|'E');


// --------------- текстови низове -------------------

fragment ESC: '\\' .;

STR_CONST: '"' (ESC | ~["\\\r\n])* '"' {{
    std::string content = getText().substr(1, getText().length() - 2);


    std::string processed = "";

    size_t symbols = 0;

    for (size_t i = 0; i < content.length(); ++i, symbols ++) {
        char current_char = content[i];

        switch (current_char) {
            case '\0': {
                set_error_message("String contains null character");
                return;
            }
            case '\t': {
                processed += "\\t";
                break;
            }
            case '\\': {
                i++; 
                char next_char = content[i];

                switch (next_char) {
                    case '\n':
                        // TODO: test 91 & 93
                        // Looks like the idea here is to save the string row by row and combine them, this way we will be able to report the right line number on errors
                        // and show the error
                    case 'n': processed += "\\n"; break;
                    case 't': processed += "\\t"; break;
                    case 'b': processed += "\\b"; break;
                    case 'f': processed += "\\f"; break;
                    case '\\': processed += "\\\\"; break;
                    case '"': processed += "\\\""; break;
                    case '\'': processed += "\\\'"; break;
                    case '\0': {
                        set_error_message("String contains escaped null character");
                        return;
                    }
                    default: processed += next_char; break;
                } 

                break;  
            }
            default: {
                processed += maybe_escape_control_char(content[i]);
            }
        }
    }

    if (symbols > MAX_STR_CONST) {
        set_error_message("String constant too long");
        return;
    }


    assoc_string_with_token(processed); 
}}
| '"' (~["\r\n])* [\r\n] {
    set_error_message("String contains unescaped new line");
};

// --------------- числа -------------------

INT_CONST: [0-9]+ { assoc_string_with_token(getText()); };

// --------------- идентификатори -------------------
TYPEID: [A-Z] [a-zA-Z0-9_]* { assoc_id_with_token(getText()); };
OBJECTID: [a-z] [a-zA-Z0-9_]* { assoc_id_with_token(getText()); };

// --------------- грешки -------------------
ERROR: . {{
    std::string invalid_symbol = maybe_escape_control_char(getText()[0]);
    set_error_message("Invalid symbol \"" + invalid_symbol + "\""); 
}};


// TODO:
// ASSIGN CASE CLASS DARROW ELSE ESAC FI IF IN
// INHERITS  ISVOID LE LET LOOP NEW NOT OBJECTID OF
// POOL  THEN TYPEID WHILE

