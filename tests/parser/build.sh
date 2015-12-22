
# TODO redefine for new directory structure

flex++ lexer.l && \
sed -i '/#define yyFlexLexer yyFlexLexer/d' ./lex.yy.cc && \
g++ -std=c++11 -I../../ext/z3/include -c lex.yy.cc ast.cpp iosacompliance.cpp smtsolver.cpp parser.cpp && \
g++ -std=c++11 main.cpp  lex.yy.o parser.o ast.o iosacompliance.o smtsolver.o -lz3 -o Parser
