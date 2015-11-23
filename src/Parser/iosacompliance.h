//==============================================================================
//
//  IOSA compliance module for FIG
//  Raul Monti
//  2015
//
//==============================================================================

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


//==============================================================================
// Class with verifying methods for compliance with IOSA models parsed into ====
// ASTs from the FIG ast module.                                            ====
//==============================================================================

class Verifier{

    parsingContext * mPc;

public:

    /**
     * @brief IOSA compliance verifier class constructor.
     * @param [in] pc A parsing context instance to fill in with parsed 
     *        information.
     */
    Verifier(parsingContext & pc);

    /**
     * @brief IOSA compliance verifier class destroyer.
     */
    virtual ~Verifier(void);

    /**
     * @brief  fully verify if @ast compliances with IOSA modeling.
     * @return 1 if it compliances.
     * @return 0 if something went wrong.
     */
    int 
    verify(AST* ast);

private:

    /**
     * @brief Fill up context @mPc for @ast.
     */
    int
    fill_maps(AST *ast);

    /**
     * @brief  check that names that should be unique really are.
     * @return 1 if no wrongly duplicated name was found.
     * @throw  ... if some wrongly duplicated name was found.
     */
    int
    names_uniqueness(AST* ast); 

    /** 
     * @brief check that input transitions have no clock to wait
     *        for. This is in compilance to IOSA first condition.
     * @return
     * @throw
    */
    int
    input_output_clocks(AST* ast);


    /**
     * @brief check that clocks are used only once as transition enable
     *        clocks, in compliance to 3rd condition for IOSA.
     * @throw
     * @result
     */
    int
    unique_outputs(AST *ast);


    /** 
     *  @brief Check compliance to condition 4 from IOSA.
     *  @Note Can only partially check due to reachability issues, thus we 
     *        report WARNINGS but we don't ensure presence of non-determinism.
     */
    int
    check_exhausted_clocks(AST *ast);


    /**
     *  @brief Type check expressions in the model.
     *  @throw String error.
     *  @param [in] ast Should be an AST instance of a parsed model.
     */
    int
    type_check(AST *ast);


    /** 
     *  @brief Return type of an expression.
     *  @param [in] module The module name to which the expression belongs to.
     *  @return
     *  @throw
     */
    Type
    get_type(AST *expr, string module);


    /**
     *  @brief Check condition 7 for IOSA.
     */
    int
    check_input_determinism(AST *ast);

};

}//namespace parser

#endif // IOSA_COMPLIANCE_H
