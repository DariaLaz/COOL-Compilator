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
    std::map<int, std::string> string_values;
    std::map<int, int> number_of_lines;

    void assoc_string_with_token(const std::string &value, int lines) {
        string_values[tokenStartCharIndex] = value;
        number_of_lines[tokenStartCharIndex] = lines;
    }

    std::string get_string_value(int token_start_char_index) {
        return string_values.at(token_start_char_index);
    }

    int maybe_get_number_of_lines_for_string(int token_start_char_index) {
        if (number_of_lines.find(token_start_char_index) == number_of_lines.end()) {
            return 0;
        }

        return number_of_lines.at(token_start_char_index);
    }

    std::string maybe_escape_control_char(char c) {
        if (c < 0x20 || c == 0x7F) {
            char buf[10];
            sprintf(buf, "<0x%02x>", c);

            return std::string(buf);
        }

        return std::string(1, c);
    }

    int count_new_lines_in_string(const std::string &s) {
        int lines = 0;
        for (char c : s) {
            if (c == '\n') {
                lines++;
            }
        }
        return lines;
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

    void set_error_message(const std::string &message, int lines) {
        setType(ERROR);
        error_messages[tokenStartCharIndex] = message;
        number_of_lines[tokenStartCharIndex] = lines;
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
//MULTILINE_COMMENT : '(*' (.)*? '*)' -> skip ;

MULTILINE_COMMENT : '(*' -> pushMode(COMMENT_MODE), skip;



// --------------- булеви константи -------------------

BOOL_CONST : TRUE  { assoc_bool_with_token(true); }
           | FALSE { assoc_bool_with_token(false); };

fragment TRUE  : 't'('r'|'R')('u'|'U')('e'|'E');
fragment FALSE : 'f'('a'|'A')('l'|'L')('s'|'S')('e'|'E');


// --------------- текстови низове -------------------

fragment ESC: '\\' .;

STR_CONST: '"' (ESC | ~["\\\n])* '"' {{
    std::string content = getText().substr(1, getText().length() - 2);


    std::string processed = "";

    size_t symbols = 0;
    size_t lines = 0;

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
            case '\f': {
                processed += "\\f";
                break;
            }
            case '\r': {
                processed += "<0x0d>";
                break;
            }
            case '\\': {
                i++; 
                char next_char = content[i];

                switch (next_char) {
                    case '\n':
                        lines++;
                    case 'n': 
                        processed += "\\n"; break;
                    case '\t':
                    case 't': 
                        processed += "\\t"; break;
                    case '\b':
                    case 'b': 
                        processed += "\\b"; break;
                    case '\f':
                    case 'f': 
                        processed += "\\f"; break;
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
        set_error_message("String constant too long", lines);
        return;
    }


    assoc_string_with_token(processed, lines); 
}}
| '"' (ESC | ~["\\\n])* [\n] {
    set_error_message("String contains unescaped new line", count_new_lines_in_string(getText()) - 1);
}
| '"' (ESC | ~["\\\n])* EOF {
    set_error_message("Unterminated string at EOF", count_new_lines_in_string(getText()));
};

// --------------- числа -------------------

INT_CONST: [0-9]+ { assoc_string_with_token(getText(), 0); };

// --------------- идентификатори -------------------

TYPEID: [A-Z] [a-zA-Z0-9_]* { assoc_id_with_token(getText()); };
OBJECTID: [a-z] [a-zA-Z0-9_]* { assoc_id_with_token(getText()); };

// --------------- грешки -------------------

ERROR: . {{
    std::string invalid_symbol = maybe_escape_control_char(getText()[0]);
    invalid_symbol = invalid_symbol == "\\" ? "\\\\" : invalid_symbol;
    set_error_message("Invalid symbol \"" + invalid_symbol + "\""); 
}};

UNMATCHED_COMMENT_CLOSE
    : '*)' { set_error_message("Unmatched *)"); }
    ;


mode COMMENT_MODE;

COMMENT_OPEN  : '(*' -> pushMode(COMMENT_MODE), skip;
COMMENT_CLOSE : '*)' -> popMode, skip;
COMMENT_TEXT  : . -> skip;
COMMENT_EOF
    : .? EOF
      {
          set_error_message("EOF in comment");
      }
    ;
