//==============================================================================
//
//    Abstract syntax tree module
//    Raul Monti
//    2015
//
//==============================================================================


#include "Ast.h"
#include <vector>
#include <string>
#include <assert.h>
#include <iostream>

using std::to_string;


//============================================================================== 
// Abstract syntax tree class implementation. ==================================
//==============================================================================


/**
 * @brief: Constructor.
 */

AST::AST(){}


//==============================================================================


/**
 * @brief: Constructor.
 */

AST::AST(int token, string lexeme, int line, int col): 
    lxm(lexeme),
    tkn(token),
    l(line),
    c(col)
{}


//==============================================================================

/**
 * @brief: Constructor.
 */

AST::AST(const AST *copy){

    assert(copy != NULL && "NULL PROBLEM");

    lxm = copy->lxm;
    tkn = copy->tkn;
    l   = copy->l;
    c   = copy->c;
	for(size_t i = 0; i < copy->branches.size(); ++i){
        branches.push_back(new AST(copy->branches[i]));
    }
}


//==============================================================================


/**
 * @brief: Destroyer. 
 */

AST::~AST(){
	for( size_t i = 0; i < branches.size(); i++){
        delete branches[i];
        branches[i] = NULL;
    }
}


//==============================================================================


/**
 * @brief: get this AST branches with token @k. 
 */

vector<AST*>
AST::get_list(int k){

    vector<AST*> result;

	for(size_t i = 0; i < branches.size(); i++){
        if (branches[i]->tkn == k) result.push_back(branches[i]);
    }

    return result;
}


//==============================================================================


/** @brief: get first child AST with token @k.
 *  @return: pointer to the found AST, NULL otherwise.
 */

AST*
AST::get_branch_k(int k){
	for(size_t i = 0; i < branches.size(); i++){
        if (branches[i]->tkn == k) return branches[i];
    }
    return NULL;
}


//==============================================================================


/**
 * @brief: get a list with the lexemes of every branch with token @k.
 */

vector<string>
AST::get_list_lexemes(int k){

    vector<string> result;

	for(size_t i = 0; i < branches.size(); i++){
        if (branches[i]->tkn == k) result.push_back(branches[i]->lxm);
    }

    return result;    
}


//==============================================================================


/**
 * @brief: recursively retrieve every lexeme from every node with token @k.
 */

vector<string>
AST::get_all_lexemes(int k){

    vector<string> result;

    if(tkn == k){
        result.push_back(lxm);
    }else{
        for(auto const &it: branches){
            vector<string> aux = it->get_all_lexemes(k);
            result.insert(result.end(), aux.begin(), aux.end());
        }
    }
    return result;    
}


//==============================================================================


/** 
 * @brief: Get the lexeme of the first child AST with token @k.
 * @return: "" if not found, the lexeme as a string otherwise.
 */

string
AST::get_lexeme(int k){

    string result("");
    if( tkn == k ){
        result = lxm;
    }else{
        for(auto const &it: branches){
            result = it->get_lexeme(k);
            if(result != ""){
                break;
            }
        }
    }

    return result;    
}


//==============================================================================


/**
 * @brief: Human readable string version of this AST Node (without childs).
 */

string
AST::p_node(void){
    string result = lxm;
    result += "(" + to_string(l) + ":" + to_string(c) + ")";
    return result;
}


//==============================================================================


/**
 * @deprecated: use get_pos() instead.
 * @brief: Printable position.
 */

string
AST::p_pos(){
    return get_pos();
}


//==============================================================================


/**
 * @brief: Printable name.
 */

string
AST::p_name(){
    return lxm;
}


//==============================================================================


/**
 * @brief: walk the tree and get every node with token @k.
 * @return: a vector of AST pointers to every node with token @k found.     
 */

vector<AST*>
AST::get_all_ast(int k){

    vector<AST*> result;
    if(tkn == k){
        result.push_back(this);
    }
	for(size_t i = 0; i < branches.size(); i++){
        vector<AST*> rec = branches[i]->get_all_ast(k);
        result.insert( result.end(), rec.begin(), rec.end() );
    }
    return result;
}


//==============================================================================

/**
 * @brief: (get all ast first found) walk the tree and get every
 *         node with token k, but stop deepening into the branch
 *         as soon as we get such a node. Then searching K2 in a tree 
 *         K1 [k2 [K2, K3], K4 []] will only return the first K2.
 * @return: a vector of AST pointers to every node with token k, first found.
 */

vector<AST*>
AST::get_all_ast_ff(int k){

    vector<AST*> result;
    if(tkn == k){
        result.push_back(this);
    }else{

		for(size_t i = 0; i < branches.size(); i++){
            vector<AST*> rec = branches[i]->get_all_ast_ff(k);
            result.insert( result.end(), rec.begin(), rec.end() );
        }
    }
    return result;
}


//==============================================================================


/**
 *  @brief: Get the i-th branch.
 */

AST*
AST::get_branch(int i){

    AST *result = NULL;
	if(i >= 0 && i < static_cast<int>(branches.size())){
        result = branches[i];
    }
    return result;
}


//==============================================================================

/** 
 * @brief: walk the tree and get the first node with token @k.
 * @return: The found node or NULL no node with token k can be found.     
 */

AST*
AST::get_first(int k){

    vector<AST*> result;
    if(tkn == k){
        return this;
    }else{
		for(size_t i = 0; i < branches.size(); i++){
            AST* first = branches[i]->get_first(k);
            if(first){
                return first;
            }
        }
    }
    return NULL;
}


//==============================================================================

/**
 * @brief: get a string with the line number of this AST instance.
 */

string 
AST::get_line(){
    string result = "0";
    if ( l != 0){
        result = to_string(l);
    }else{
		for( size_t i = 0; i < branches.size() && result == "0"; ++i){
            result = branches[i]->get_line();
        }
    }
    return result;
}

//==============================================================================

/**
 * @brief: get a string with the line number of this AST instance.
 */

string 
AST::get_column(){
    string result = "0";
    if ( c != 0){
        result = to_string(c);
    }else{
		for( size_t i = 0; i < branches.size() && result == "0"; ++i){
            result = branches[i]->get_column();
        }
    }
    return result;
}


//==============================================================================

/** 
 * @brief: get the line and column of this AST instance, separated by a colon.
 */

string
AST::get_pos(){
    return get_line() + ":" + get_column();
}

//==============================================================================

/**
 * TODO
 */
string
AST::toString(){
    string result = lxm;
    for(const auto &it: branches){
        string aux = it->toString();
        result.insert(result.end(),aux.begin(),aux.end());
    }
    return result;
}


//==============================================================================

bool
operator==(AST &ast1, AST &ast2){

if( ast1.lxm == ast2.lxm &&
    ast1.tkn == ast2.tkn &&
    ast1.branches.size() == ast2.branches.size())
{
	for(size_t i = 0; i < ast1.branches.size(); ++i){
        if(!(*ast1.branches[i] == *ast2.branches[i])){
            return false;
        }
    }
    return true;
}
return false;
}

bool
operator!=(AST &ast1, AST &ast2){
    return !(ast1==ast2);
}



