/**

    IOSA compliance module for FIG
    Raul Monti
    2015

**/
#ifndef IOSA_COMPLIANCE_H
#define IOSA_COMPLIANCE_H


#include<vector>
#include<string>
#include<set>
#include<map>
#include "ast.h"


using namespace std;


/** Class with verifying methods for 
    compliance with IOSA models parsed 
    into ASTs from the FIG ast module.
**/

class Verifier{

    /**/
    typedef enum    { mARIT
                    , mBOOL
                    , mNOTYPE
                    } Type; 

    string error_list;
    // Map from module name to variable name to type:
    map<string, map<string,Type>> typeMap;
    // Map from modules to clock names:
    map<string,set<string>> clckMap;

public:

    /* @verify: fully verify if @ast compliances with IOSA modeling.
       @returns: 1 if it compliances, 0 otherwise.
    */
    int 
    verify(AST* ast);

private:

    /* @fill_maps: fill up typeMap and clckMap for @ast.
    */
    int
    fill_maps(AST *ast);

    /* @names_uniqueness: check that names that should be unique really are.
       @return: 1 if no wrongly duplicated name was found.
       @throw: ... if some wrongly duplicated name was found.
    */
    int
    names_uniqueness(AST* ast); 

    /* @instantaneous_input: check that input transitions have no clock to wait
                             for. This is in compilance to IOSA first condition.
       @return:
       @throw:
    */
    int
    input_output_clocks(AST* ast);

    /* @unique_ouputs: check that clocks are used only once as transition enable
                       clocks, in compliance to 3rd condition for IOSA.
       @throw:
       @result:
    */
    int
    unique_outputs(AST *ast);

    /* @type_check: Type check every expression in @ast. */
    int
    type_check(AST *ast);

    /* @get_type: Return type of an expression.
       @module: the module name (key for the typeMap).
       @return:
       @throw:
    */
    Type
    get_type(AST *expr, string module);

};

#endif // IOSA_COMPLIANCE_H
