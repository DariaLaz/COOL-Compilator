parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program: (class ';')+ ;

class: CLASS TYPEID (INHERITS TYPEID)? '{' feature* '}' ;

feature: attribute | method;

attribute: OBJECTID':' TYPEID ';' ;

method: OBJECTID'('(formal (',' formal)*)?')'':' TYPEID '{' expresion '}' ';' ;

formal: OBJECTID':' TYPEID;

expresion: assign | staticDispatch | dispatch | condition | while | block | letIn | typcase | object ;

assign: OBJECTID ASSIGN object;
object: OBJECTID;

staticDispatch: object'@'TYPEID'.'OBJECTID'('(argument (',' argument)*)?')';

dispatch: OBJECTID'('(argument (',' argument)*)?')';

argument: object;

condition: IF object THEN object ELSE object FI;

while: WHILE object LOOP object POOL;

block: '{' (expresion';')+ '}';

letIn: LET letInArg (',' letInArg)* IN var;
letInArg: OBJECTID ':' TYPEID (assignExpresion)?;
assignExpresion: ASSIGN var;

var: string | int | bool | object;

string: STR_CONST;
int: INT_CONST;
bool: BOOL_CONST;

typcase: CASE object OF (typcaseBranch)+ ESAC;
typcaseBranch: OBJECTID ':' TYPEID DARROW object ';';