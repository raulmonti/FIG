To test the parser:

lex my_lexer.lex && g++ -std=c++11 -c lex.yy.c && g++ -std=c++11 my_parser.cpp lex.yy.o -o Parser && ./Parser
