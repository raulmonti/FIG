/**

    IOSA compliance module for FIG
    Raul Monti
    2015

**/

#include <set>
#include "parser.h"
#include "iosacompliance.h"



using namespace std;

/* @verify: fully verify if @ast compliances with IOSA modeling.
   @returns: 1 if it compliances, 0 otherwise.
*/
int 
Verifier::verify(AST* ast){
    return names_uniqueness(ast);
}



/* @names_uniqueness: check that names that should be unique really are.
   @return: 1 if no wrongly duplicated name was found.
   @throw: ... if some wrongly duplicated name was found.
*/
int
Verifier::names_uniqueness(AST* ast){

    set<string> names;


    /* Unique modules names: */
    vector<AST*> mAst = ast->get_list(parser::_MODULE);
    for(int i = 0; i < mAst.size();i++){
        string name = mAst[i]->get_lexeme(parser::_NAME);
        if (names.insert(name).second == false){
            throw (string("Duplicated module name '")
                             + name
                             + string("'") ) ;
        } 
    }

    names.clear();

    /* Unique clock names: */
    for(int i = 0; i < mAst.size();i++){ // For each module
        vector<AST*> cSecs = mAst[i]->get_list(parser::_CLOCKSEC);
        if(!cSecs.empty()){ // If the module has a clock section
            AST* clksec = cSecs[0];
            vector<AST*> clks = clksec->get_list(parser::_CLOCK);
            for(int j = 0; j < clks.size(); j++){ // for each clock
                string name = clks[j]->get_lexeme(parser::_NAME);
                if (names.insert(name).second == false){
                throw (string("Duplicated clock name '")
                                 + name
                                 + string("'") ) ;
                } 
            }
        }
    }

    
}

