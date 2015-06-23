/* My Lexer 19-06-2015
   Using flex
   Raul Monti
*/

%{
#include<iostream>
#include<vector>
#include<string>
#include"exceptions.h"
#include"my_lexer.h"


std::vector<int> vec;
std::vector<string> ii;
%}


/* This tells flex to read only one input file */
%option noyywrap
 
%%

[0-9]+([\.][0-9]+)?  {
            std::cout << "Saw a number: " << yytext << std::endl;
            vec.push_back(NUM);
            ii.push_back(string(yytext));
        }


 
[a-zA-Z]+ {
              std::cout << "Saw a name: " << yytext << std::endl;
              vec.push_back(NAME);
              ii.push_back(string(yytext));
          }

[ \t]+    {    std::cout << "Saw a WS: <" << yytext << ">" << std::endl;
               vec.push_back(WS);
               ii.push_back(string(yytext));
          }

\n        { 
              std::cout << "Saw a NL: <\n>" << std::endl;
              vec.push_back(NL);
              ii.push_back(string(yytext));
          }

[^[:alnum:]] {
                throw new Badcharfound(yytext);
             }

%%

/*** C Code section ***/


int Lexer::lex (string str)
{

    YY_BUFFER_STATE buff;
    std::cout << "Lexing <<" << str 
              << ">> Of length: " << str.size() << std::endl;
    
    try{
        buff = yy_scan_bytes(str.c_str(), str.size());
        yylex();
    }catch(Badcharfound* e){

        std::cout << e->what() << '\n';

    }
    yy_delete_buffer(buff);
    
    return 0;
}

