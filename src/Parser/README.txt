#############################################
# PARSER MODULE FOR FIG
#############################################

#############################################
# REQUIRES
#############################################

1) flex (tested working with version 2.5.35)
2) g++  (tested working with version 4.8.4)

#############################################
# COMPILING (and running afterwards):
#############################################

1) flex++ lexer.l 
2) Remove line 22 of lex.yy.cc (#define yyFLexLexer yyFLexLexer)
3) g++ -c lex.yy.cc && g++ -c parser.cpp -std=c++11 && g++ main.cpp lex.yy.o parser.o -o Parser
4) ./Parser test.fig



#############################################
# TODO
#############################################

/*
TODO 	parametric skip whites for Parser class.
SOLVED 	column count for error reporting.
TODO 	test suit.
TODO 	reset parser method.
*/
