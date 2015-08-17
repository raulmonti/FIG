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
    for( int i = 0; i < branches.size(); i++){
        delete branches[i];
        branches[i] = NULL;
    }
}

vector<AST*>
AST::get_list(int k){

    vector<AST*> result;

    for(int i = 0; i < branches.size(); i++){
        if (branches[i]->tkn == k) result.push_back(branches[i]);
    }

    return result;
}

/* @get_first: get first child AST with token k.
*/
AST*
AST::get_first(int k){
    for(int i = 0; i < branches.size(); i++){
        if (branches[i]->tkn == k) return branches[i];
    }
    return NULL;
}

vector<string>
AST::get_list_lexemes(int k){

    vector<string> result;

    for(int i = 0; i < branches.size(); i++){
        if (branches[i]->tkn == k) result.push_back(branches[i]->lxm);
    }

    return result;    
}


string
AST::get_lexeme(int k){

    string result("");

    for(int i = 0; i < branches.size(); i++){
        if (branches[i]->tkn == k){
            result = branches[i]->lxm; 
            break;
        }
    }

    return result;    
}

/* Human readable string version of this AST Node (without childs). */
string
AST::p_node(void){
    string result = lxm;
    result += "(" + to_string(l) + ":" + to_string(c) + ")";
    return result;
}

/* Printable position. */
string
AST::p_pos(){
    return to_string(l) + ":" + to_string(c);
}

/* Printable name. */
string
AST::p_name(){
    return lxm;
}


/* @get_all_ast: walk the tree and get every node with token k.
   @return: a vector of AST pointers to every node with token k.     
*/
vector<AST*>
AST::get_all_ast(int k){

    vector<AST*> result;
    if(tkn == k){
        result.push_back(this);
    }
    for(int i = 0; i < branches.size(); i++){
        vector<AST*> rec = branches[i]->get_all_ast(k);
        result.insert( result.end(), rec.begin(), rec.end() );
    }
    return result;
}

/* @get_branch: Get the ith child. */
AST*
AST::get_branch(int i){

    AST *result = NULL;
    if(i >= 0 && i < branches.size()){
        result = branches[i];
    }
    return result;
}
