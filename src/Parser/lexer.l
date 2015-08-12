/**

    Lexing rules for flex lexer.
    Raul Monti
    2015
    FIG project.

**/


%{
#include <iostream>
#include <sstream>
#include <string>
#include "parser.h"
#include "exceptions.h"
#include "debug.h" 
%}

/* This tells flex to read only one input file */
%option noyywrap
%option yylineno


%%

-?[0-9]+([\.][0-9]+)?     { return parser::NUM; }

MODULE      { return parser::KMOD; }

ENDMODULE   { return parser::KEMOD; }

LABELS      { return parser::KLBL; }

CLOCKS      { return parser::KCS; }

VARS        { return parser::KVS; }

TRANS       { return parser::KTS; }

Normal      { return parser::KNDIST; }

Exponential { return parser::KEDIST; }

Uniform     { return parser::KUDIST; }

("Int"|"Float")         { return parser::KVTYPE; } 

("input"|"output")      { return parser::LDIR; }

("true"|"false")        { return parser::BOOLV; }

[a-zA-Z][a-zA-Z0-9]*    { return parser::NAME; }

[ \t]+      { return parser::WS; }

\n          { return parser::NL; }

:           { return parser::CLN; }

;           { return parser::SCLN; }

,           { return parser::CMM; }

[{]         { return parser::OB; }

[}]         { return parser::CB; }

[[]         { return parser::OBT; }

[]]         { return parser::CBT; }

[(]         { return parser::OP; }

[)]         { return parser::CP; }

(">>")      { return parser::ARROW; }

("<="|">="|"<"|">")             { return parser::COP; }

("=="|"!=")                     { return parser::BOP; }

("/"|"+"|"-"|"*"|"%")           { return parser::MOP; }

("=")       { return parser::ASSIG; }

("..")      { return parser::RNG; }

("&&"|"||") { return parser::BINOP; }

("?")       { return parser::QMARK; }

("!")       { return parser::EMARK; }

[^[:alnum:]]    {
                    stringstream ss;
                    ss << " at line " << yylineno << endl;
                    string str = string(yytext) + ss.str();
                    throw new Badcharfound(str);
                }


%%

