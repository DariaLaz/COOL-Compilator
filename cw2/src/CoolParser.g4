parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program: (class ';')+ ;

class: CLASS TYPEID '{' feature* '}' ;

feature: attribute | method;

attribute: OBJECTID':' TYPEID ';' ;

method: OBJECTID'('(formal (',' formal)*)?')'':' TYPEID '{' expresion '}' ';' ;

formal: OBJECTID':' TYPEID;

expresion: assign | staticDispatch | dispatch | condition | object ;

assign: OBJECTID ASSIGN object;
object: OBJECTID;

staticDispatch: object'@'TYPEID'.'OBJECTID'('(argument (',' argument)*)?')';

dispatch: OBJECTID'('(argument (',' argument)*)?')';

argument: object;

condition: IF object THEN object ELSE object FI;