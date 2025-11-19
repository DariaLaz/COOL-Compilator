parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program: (class ';')+ ;

class: CLASS TYPEID '{' feature* '}' ;

feature: attribute | method;

attribute: OBJECTID':' TYPEID ';' ;

method: OBJECTID'('(formal (',' formal)*)?')'':' TYPEID '{' expresion '}' ';' ;

formal: OBJECTID':' TYPEID;

expresion: assign | staticDispatch | object ;

assign: OBJECTID ASSIGN object;
object: OBJECTID;

staticDispatch: object'@'TYPEID'.'OBJECTID'('(argument (',' argument)*)?')';

argument: object;