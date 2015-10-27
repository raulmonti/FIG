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
#include "parsingContext.h"
#include "smtsolver.h"
#include "config.h"


using namespace std;

namespace parser{

/** Class with verifying methods for 
    compliance with IOSA models parsed 
    into ASTs from the FIG ast module.
**/


class Verifier{

    parsingContext * mPc;

public:

    Verifier(parsingContext & pc);
    virtual ~Verifier(void);

    /* @verify: fully verify if @ast compliances with IOSA modeling.
       @returns: 1 if it compliances, 0 otherwise.
    */
    int 
    verify(AST* ast);

private:

    /* @fill_maps: fill up context @mPc for @ast.
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


    /* @check_exhausted_clocks: check compliance to condition 4 from IOSA.
       @Note: can only partially check due to reachability issues, thus we 
              report WARNINGS but we don't ensure presence of non-determinism.
    */
    int
    check_exhausted_clocks(AST *ast);


    /* @type_check: Type check every expression in @ast. */
    int
    type_check(AST *ast);


    /* @get_type: Return type of an expression.
       @module: the module name to which the expression belongs to.
       @return:
       @throw:
    */
    Type
    get_type(AST *expr, string module);


    /*
    */
    int
    check_input_determinism(AST *ast);

};

}//namespace parser

#endif // IOSA_COMPLIANCE_H
