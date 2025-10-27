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
}

// --------------- прости жетони -------------------

SEMI   : ';';
DARROW : '=>';

WS : [ \r\n\u000D]+ -> skip ;

// Добавете тук останалите жетони, които представляват просто текст.


// --------------- булеви константи -------------------

BOOL_CONST : TRUE  { assoc_bool_with_token(true); }
           | FALSE { assoc_bool_with_token(false); };

fragment TRUE  : 't'('r'|'R')('u'|'U')('e'|'E');
fragment FALSE : 'f'('a'|'A')('l'|'L')('s'|'S')('e'|'E');

// --------------- коментари -------------------

ONE_LINE_COMMENT : '--' ~[\n]* -> skip ;

MULTILINE_COMMENT : '(*' ( MULTILINE_COMMENT | . )*? '*)' -> skip ;


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






// --------------- текстови низове -------------------

STR_CONST: '"' (~["])* '"' {{
    std::string content = getText().substr(1, getText().length() - 2);
    std::string processed = "";

    for (size_t i = 0; i < content.length(); ++i) {
        if (content[i] == '\\') {
            if (i + 1 >= content.length()) {
                // TODO: maybe handle error for invalid escape sequence
                break;
            }

            i++; 
            char next_char = content[i];

            switch (next_char) {
                case 'n': processed += "\\n"; break;
                case 't': processed += "\\t"; break;
                case 'b': processed += "\\b"; break;
                case 'f': processed += "\\f"; break;
                case '\\': processed += "\\\\"; break;
                case '"': processed += "\\\""; break;
                case '\'': processed += "\\\'"; break;
                default: processed += next_char; break;
            }
        } else {
            processed += content[i];
        }
    }

    assoc_string_with_token(processed); 
}};

// TODO:
// ASSIGN BOOL_CONST CASE CLASS DARROW ELSE ESAC FI IF IN
// INHERITS INT_CONST ISVOID LE LET LOOP NEW NOT OBJECTID OF
// POOL STR_CONST THEN TYPEID WHILE

