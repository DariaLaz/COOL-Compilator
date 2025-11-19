parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program: (class ';')+ ;

class: CLASS TYPEID (INHERITS TYPEID)? '{' feature* '}' ;

feature: attribute | method;

attribute: OBJECTID':' TYPEID ';' ;

method: OBJECTID'('(formal (',' formal)*)?')'':' TYPEID '{' expresion '}' ';' ;

formal: OBJECTID':' TYPEID;

expresion: equal | assign;

equal: mathExpresion ('=' mathExpresion)*;

unaryValue: neg | atom;

atom: int | string | bool | object | staticDispatch | dispatch | block | letIn | condition | while | typcase | '(' expresion ')';

assign: OBJECTID ASSIGN expresion;
object: OBJECTID;

staticDispatch: object'@'TYPEID'.'OBJECTID'('(argument (',' argument)*)?')';

dispatch: OBJECTID'('(argument (',' argument)*)?')';

argument: expresion;

condition: IF expresion THEN expresion ELSE expresion FI;

while: WHILE expresion LOOP expresion POOL;

block: '{' (expresion';')+ '}';

letIn: LET letInArg (',' letInArg)* IN expresion;
letInArg: OBJECTID ':' TYPEID (assignExpresion)?;
assignExpresion: ASSIGN expresion;


string: STR_CONST;
int: INT_CONST;
bool: BOOL_CONST;

typcase: CASE object OF (typcaseBranch)+ ESAC;
typcaseBranch: OBJECTID ':' TYPEID DARROW object ';';

mathExpresion: additionExpresion;

additionExpresion: multiplicationExpresion (additionOperant multiplicationExpresion)*;
additionOperant: '+' | '-';

multiplicationExpresion: unaryValue (multiplicationOperant unaryValue)*;
multiplicationOperant: '*' | '/';


neg: '~''(' expresion ')';