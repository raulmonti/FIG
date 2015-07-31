/**

    IOSA compliance module for FIG
    Raul Monti
    2015

**/
#ifndef IOSA_COMPLIANCE_H
#define IOSA_COMPLIANCE_H


#include "ast.h"

using namespace std;


/** Class with verifying methods for 
    compliance with IOSA models parsed 
    into ASTs from the FIG ast module.
**/

class Verifier{


public:

    /* @verify: fully verify if @ast compliances with IOSA modeling.
       @returns: 1 if it compliances, 0 otherwise.
    */
    int 
    verify(AST* ast);

private:

    /* @names_uniqueness: check that names that should be unique really are.
       @return: 1 if no wrongly duplicated name was found.
       @throw: ... if some wrongly duplicated name was found.
    */
    int
    names_uniqueness(AST* ast); 

};

#endif // IOSA_COMPLIANCE_H
