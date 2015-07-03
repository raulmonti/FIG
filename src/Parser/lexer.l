
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


%%

[0-9]+([\.][0-9]+)? {
            __debug__("Saw a Number: ");__debug__(yytext);__debug__("\n");
            return parser::NUM;
        }

MODULE  {
            __debug__("Saw a Module: ");__debug__(yytext);__debug__("\n");
            return parser::KMOD;
        }

CLKS    {
            __debug__("Saw a Clock: ");__debug__(yytext);__debug__("\n");
            return parser::KCS;
        }

VARS    {
            __debug__("Saw a Variable: ");__debug__(yytext);__debug__("\n");
            return parser::KVS;
        }

TRANS   {
            __debug__("Saw a Transition: ");__debug__(yytext);__debug__("\n");
            return parser::KTS;
        }

[a-zA-Z]+ {
            __debug__("Saw a Name: ");__debug__(yytext);__debug__("\n");
            return parser::NAME;
        }

[ \t]+ {    
            __debug__("Saw a WS: ");__debug__(yytext);__debug__("\n");
            return parser::WS;
        }

\n  { 
            __debug__("Saw a NL: ");__debug__(yytext);__debug__("\n");
            return parser::NL;
        }

:       {
            __debug__("Saw a Colon: ");__debug__(yytext);__debug__("\n");
            return parser::CLN;
        }
;       {
            __debug__("Saw a SemiColon: ");__debug__(yytext);__debug__("\n");
            return parser::SCLN;
        }
[^[:alnum:]]    {
                    stringstream ss;
                    ss << " at line " << yylineno << endl;
                    string str;
                    ss >> str;
                    str = string(yytext) + str;
                    throw new Badcharfound(str);
                }


%%

