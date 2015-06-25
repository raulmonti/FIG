
%{
#include <iostream>
#include "parser.h"
#include "exceptions.h"
// Because flex++ builds a line #define yyFlexLexer yyFlexLexer: 
#undef yyFlexLexer 
%}

/* This tells flex to read only one input file */
%option noyywrap

%%

[0-9]+([\.][0-9]+)? {
        std::cout << "Saw a number: " << yytext << std::endl;
        return parser::NUM;
    }

 
[a-zA-Z]+ {
        std::cout << "Saw a name: " << yytext << std::endl;
        return parser::NAME;
    }

[ \t]+ {    
        std::cout << "Saw a WS: <" << yytext << ">" << std::endl;
        return parser::WS;
    }

\n  { 
        std::cout << "Saw a NL: <\n>" << std::endl;
        return parser::NL;
    }

[^[:alnum:]] {
        throw new Badcharfound(yytext);
    }


%%

