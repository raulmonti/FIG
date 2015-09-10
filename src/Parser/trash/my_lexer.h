#ifndef MY_LEXER_H
#define MY_LEXER_H

#include<vector>
#include<string>

using namespace std;

extern vector<int> vec;
extern vector<string> ii;

typedef enum { NUM, NAME, WS, NL, INT, MEOF} Symbol;

class Lexer
{

public:

    int lex (string str);


};




#endif
