/**

    Abstract syntax tree module
    Raul Monti
    2015

**/

#include "ast.h"
#include <vector>
#include <string>

/** Abstract syntax tree class implementation. **/

AST::AST(){};

AST::AST(int token, string lexeme, int line, int col): 
    lxm(lexeme),
    tkn(token),
    l(line),
    c(col)
{};


AST::~AST(){
    for( int i = 0; i < list.size(); i++){
        delete list[i];
        list[i] = NULL;
    }
}

vector<AST*>
AST::get_list(int k){

    vector<AST*> result;

    for(int i = 0; i < list.size(); i++){
        if (list[i]->tkn == k) result.push_back(list[i]);
    }

    return result;
}

vector<string>
AST::get_list_lexemes(int k){

    vector<string> result;

    for(int i = 0; i < list.size(); i++){
        if (list[i]->tkn == k) result.push_back(list[i]->lxm);
    }

    return result;    
}


string
AST::get_lexeme(int k){

    string result("");

    for(int i = 0; i < list.size(); i++){
        if (list[i]->tkn == k){
            result = list[i]->lxm; 
            break;
        }
    }

    return result;    
}
