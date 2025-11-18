parser grammar CoolParser;

options { tokenVocab=CoolLexer; }

program: (class ';')+ ;

class: CLASS TYPEID '{' feature* '}' ;

feature: attribute | method;

attribute: OBJECTID':' TYPEID ';' ;

method: OBJECTID'('')'':' TYPEID '{' expresion '}' ';' ;

expresion: OBJECTID ;