
************* B R O C K E N ! ! *************
Dont use get an other version :S


#############################################
# PARSER MODULE FOR FIG
#############################################

#############################################
# REQUIRES
#############################################

1) flex (tested working with version 2.5.35)
2) g++  (tested working with version 4.8.4)

#############################################
# COMPILING (and running test):
#############################################

1) flex++ lexer.l 
2) Remove line 22 of lex.yy.cc (#define yyFLexLexer yyFLexLexer)
3) g++ -std=c++11 -c lex.yy.cc ast.cpp && g++ -c parser.cpp -std=c++11 && g++ -std=c++11 main.cpp lex.yy.o parser.o ast.o -o Parser
4) ./Parser test.fig

* Or the fast way:
flex++ lexer.l && sed -i '/#define yyFlexLexer yyFlexLexer/d' ./lex.yy.cc && g++ -std=c++11 -c lex.yy.cc ast.cpp && g++ -c parser.cpp -std=c++11 && g++ -std=c++11 main.cpp lex.yy.o parser.o ast.o -o Parser


#############################################
# USE EXAMPLE
#############################################

Read main.cpp file for an example.

#############################################
# TODO
#############################################

/*
TODO 	parametric skip whites for Parser class.
SOLVED 	column count for error reporting.
TODO 	test suit.
TODO 	reset parser method.
SOLVED  leack check
TODO    Idea: parser with templates
TODO    Idea: Buil the AST in a single bunch of method like Accept or Expect,
        instead of ditributing this work in between every grammar rule. A 
        stack may be needed for this.
TODO    Idea: Exception handling using numbers and table with 
TODO    Check label section in ioscompilance or better remove label section as
        it seams useless.
TODO    Allow expressions as range limits.
FIXME   Syntax shouldn't include the explicit reset of clocks. We should ensure
        condition 4 for IOSA reseting clocks that are about to be used.
*/
