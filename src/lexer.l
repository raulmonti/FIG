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
#include "config.h" 
%}



/* This tells flex to read only one input file */
%option noyywrap
%option yylineno


%%

"/*"((("*"[^/])?)|[^*])*"*/" { return parser::COMMENT; }

[0-9]+([\.][0-9]+)?     { return parser::NUM; }

module          { return parser::KMOD; }

("const")       {return parser::KCONST; }

("endmodule")  { return parser::KEMOD; }

init        { return parser::KINIT; }

normal        { return parser::KNDIST; }

exponential         { return parser::KEDIST; }

uniform         { return parser::KUDIST; }

("int")     { return parser::ITYPE; }
("bool")    { return parser::BTYPE; }

("clock")       {return parser::KCLOCK; }

("true"|"false")        { return parser::BOOLV; }


[a-zA-Z][a-zA-Z0-9]*(\.[a-zA-Z0-9]+)?'   { return parser::XNAME; }

[a-zA-Z][a-zA-Z0-9]*(\.[a-zA-Z0-9]+)?    { return parser::NAME; }

[ \t]+      { return parser::WS; }

\n          { return parser::NL; }

:           { return parser::CLN; }

;           { return parser::SCLN; }

,           { return parser::CMM; }

@           { return parser::AT; }

'           { return parser::AP; }

[{]         { return parser::OB; }

[}]         { return parser::CB; }

[[]         { return parser::OBT; }

[]]         { return parser::CBT; }

[(]         { return parser::OP; }

[)]         { return parser::CP; }

("->")              { return parser::ARROW; }
("<="|">="|"<"|">") { return parser::COP; }
("=="|"!=")         { return parser::BOP; }
("+")               { return parser::PLUS; }
("-")               { return parser::MINUS; }

("*"|"/"|"%")   { return parser::DIVOP; }
("=")           { return parser::ASSIG; }
("..")          { return parser::RNG; }
("&")           { return parser::AMP; }
("|")           { return parser::MID; }
("?")           { return parser::QMARK; }
("!")           { return parser::EMARK; }

[^[:alnum:]]    {
                    stringstream ss;
                    ss << " at line " << yylineno <<  endl;
                    string str = string(yytext) + ss.str();
                    throw new BadSymbol(str.c_str());
                }


%%

