/* -*- C++ -*- */
%{ 
#include <cerrno>
#include <climits>
#include <cfloat>
#include <cstdlib>  
#include <string>
#include <iostream>
#include "ModelAST.h"
#include "ModelParser.hpp"

#undef yywrap
#define yywrap() 1
    using namespace ModelParserGen;
    static ModelParserGen::location loc;
    int parse_int();
    float parse_float();
    void syntax_error(ModelParserGen::location loc, const char * msg);
%}

%option noyywrap nounput batch noinput 
 //option debug

id    [a-zA-Z][a-zA-Z_0-9]*
int   [0-9]+
float [0-9]+[\.][0-9]+
blank  [ \t]

%{
    // Code run each time a pattern is matched.
#define YY_USER_ACTION  loc.columns(yyleng);
%}

%x comment

%%

%{
  // Code run each time yylex is called.
        loc.step();
%}

{blank}+   loc.step();
[\n]+      loc.lines(yyleng);
"//".*     loc.step();
"/*"       loc.step(); BEGIN(comment);
<comment>[^*\n]* loc.step();
<comment>"*"+[^*/\n]* loc.step();
<comment>\n        loc.step(); loc.lines(1);
<comment>"*"+"/"   loc.step(); BEGIN(INITIAL);
"module"      return ModelParser::make_MODULE(loc);
"endmodule"   return ModelParser::make_ENDMODULE(loc);
"properties"     return ModelParser::make_PROP(loc);
"endproperties"  return ModelParser::make_ENDPROP(loc);
";"           return ModelParser::make_COMMA(loc);
"int"         return ModelParser::make_TINT(loc);
"bool"        return ModelParser::make_TBOOL(loc);
"float"       return ModelParser::make_TFLOAT(loc);
"const"       return ModelParser::make_CONST(loc);
"["           return ModelParser::make_LBRACKET(loc);
"]"           return ModelParser::make_RBRACKET(loc);
"("           return ModelParser::make_LPAREN(loc);
")"           return ModelParser::make_RPAREN(loc);
"{"           return ModelParser::make_LBRACE(loc);
"}"           return ModelParser::make_RBRACE(loc);
"clock"       return ModelParser::make_CLOCK(loc);
"="           return ModelParser::make_EQ(loc);
"=="          return ModelParser::make_EQQ(loc);
"!="          return ModelParser::make_NEQ(loc);
"<"           return ModelParser::make_LT(loc);
">"           return ModelParser::make_GT(loc);
"<="          return ModelParser::make_LE(loc);
">="          return ModelParser::make_GE(loc);
"=>"          return ModelParser::make_IMPLY(loc);
":"           return ModelParser::make_SEMICOLON(loc);
"'"           return ModelParser::make_SQUOTE(loc);
","           return ModelParser::make_COLON(loc);
".."          return ModelParser::make_DOTDOT(loc);
"@"           return ModelParser::make_AT(loc);
"erlang"      return ModelParser::make_ERLANG(loc);
"normal"      return ModelParser::make_NORMAL(loc);
"uniform"     return ModelParser::make_UNIFORM(loc);
"exponential" return ModelParser::make_EXP(loc);
"lognormal"   return ModelParser::make_LOGNORMAL(loc);
"gamma"       return ModelParser::make_GAMMA(loc);
"rayleigh"    return ModelParser::make_RAYLEIGH(loc);
"weibull"     return ModelParser::make_WEIBULL(loc);
"dirac"       return ModelParser::make_DIRAC(loc);
"init"        return ModelParser::make_INIT(loc);
"!"           return ModelParser::make_ADM(loc);
"?"           return ModelParser::make_QMARK(loc);
"+"           return ModelParser::make_PLUS(loc);
"*"           return ModelParser::make_TIMES(loc);
"-"           return ModelParser::make_MINUS(loc);
"%"           return ModelParser::make_MOD(loc);
"/"           return ModelParser::make_DIV(loc);
"&"           return ModelParser::make_AND(loc);
"->"          return ModelParser::make_ARROW(loc);
"true"        return ModelParser::make_TRUE(loc);
"false"       return ModelParser::make_FALSE(loc);
"|"           return ModelParser::make_MID(loc);
"P"           return ModelParser::make_PROPT(loc);
"S"           return ModelParser::make_PROPS(loc);
"B"           return ModelParser::make_PROPB(loc);
"U"           return ModelParser::make_UNTIL(loc);
"floor"       return ModelParser::make_FLOOR(loc);
"ceil"        return ModelParser::make_CEIL(loc);
"abs"         return ModelParser::make_ABS(loc);
"sgn"         return ModelParser::make_SGN(loc);
"log"         return ModelParser::make_LOG(loc);
"max"         return ModelParser::make_MAX(loc);
"min"         return ModelParser::make_MIN(loc);
"pow"         return ModelParser::make_POW(loc);
"_"           return ModelParser::make_UNDERSCORE(loc);
"fsteq"       return ModelParser::make_FSTEQ(loc);
"lsteq"       return ModelParser::make_LSTEQ(loc);
"rndeq"       return ModelParser::make_RNDEQ(loc);
"minfrom"     return ModelParser::make_MINFROM(loc);
"maxfrom"     return ModelParser::make_MAXFROM(loc);
"sumfrom"     return ModelParser::make_SUMFROM(loc);
"sumkmax"     return ModelParser::make_SUMKMAX(loc);
"consec"      return ModelParser::make_CONSEC(loc);
"broken"      return ModelParser::make_BROKEN(loc);
"fstexclude"  return ModelParser::make_FSTEXCLUDE(loc);
{id}       return ModelParser::make_ID(yytext, loc);
{int}      return ModelParser::make_INTL(parse_int(), loc);
{float}    return ModelParser::make_FLOATL(parse_float(), loc);
.          syntax_error(loc, yytext);
<<EOF>>    return ModelParser::make_END(loc);

%%

int parse_int() {
  errno = 0;
  long n = strtol(yytext, NULL, 10);
  if (!(INT_MIN <= n && n <= INT_MAX && errno != ERANGE))
    std::cout << "OUT OF RANGE" << std::endl;
  return static_cast<int>(n);
}

float parse_float() {
	// #TODO: check precision, overflow, ....
    float n = strtof(yytext, NULL);
    return n;
}

void syntax_error(ModelParserGen::location loc, const char * msg) {
    std::cerr << "Unexpected";
}

void ModelAST::scan_begin(FILE *file) {
  yyin = file;
}

void ModelAST::scan_end() {
  fclose (yyin);
  yylex_destroy();
}

