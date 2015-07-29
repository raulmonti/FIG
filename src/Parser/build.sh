flex++ lexer.l && sed -i '/#define yyFlexLexer yyFlexLexer/d' ./lex.yy.cc && g++ -std=c++11 -c lex.yy.cc && g++ -c parser.cpp -std=c++11 && g++ -std=c++11 main.cpp lex.yy.o parser.o -o Parser
