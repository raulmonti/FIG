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
#include<utility> // for pair type
#include "Ast.h"
#include "config.h"
#include "Parser.h"
#include <z3++.h>

using namespace std;
using namespace parser;
using namespace z3;



namespace parser{

/**
 * IOSA compliance verification class.
 */
class Verifier{

    /* Map from variable/clock/const name to (type, module) */
    parsingContext mPc;

public:

    /**
     * @brief IOSA compliance verifier class constructor.
     * @param [in] pc A parsing context instance to fill in with parsed 
     *        information.
     */
    Verifier() {}

    /**
     * @brief IOSA compliance verifier class destroyer.
     */
    virtual ~Verifier(void) {}

    /**
     * @brief  fully verify if @ast compliances with IOSA modeling.
     */
    void 
    verify( AST* ast, const parsingContext);

private:

    /**
     *
     */
    bool
    is_clock(AST* c);


    /**
     *
     */
    z3::expr
    limits2expr(AST* ast, z3::context &c);

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
    void
    unique_outputs(AST *ast);


    /** 
     *  @brief Check compliance to condition 4 from IOSA.
     *  @Note Can only partially check due to reachability issues, thus we 
     *        report WARNINGS but we don't ensure presence of non-determinism.
     */
    void
    check_exhausted_clocks(AST *ast);


    /**
     *  @brief Type check expressions in the model.
     *  @throw String error.
     *  @param [in] ast Should be an AST instance of a parsed model.
     */
    int
    type_check(AST *ast);


    /**
     *  @brief Check condition 7 for IOSA.
     */
    void
    check_input_determinism(AST *ast);

    /**
     * @brief Check that every constant definition in @ast is correct, i.e 
     *        it does not depend of variables, and it is free of circular
     *        dependencies.
     */
    void
    check_constants(AST* ast);


};


bool
is_var(AST* c, const parsingContext &pc);

/** 
 *  @brief Return type of an expression.
 *  @param
 *  @return
 *  @throw
 */
Type
get_type(AST *expr, const parsingContext &pc);


/** TODO
*/
string
solve_const_expr(AST* ex, const parsingContext &pc);

/* @brief:   Return a z3 expression corresponding to a boolean formula
             represented in an AST member. Correctly fill in the z3::context 
             member, in order to be able to sat-check over the resulting
             expression afterwards.
   @formula: The AST member to be translated into a z3::expr.
   @module:  The name of the module over which to interpret @formula.
   @c:       The context member to be filled up.
   @pc:      A parsingContext member from which to take type information for
             each variable in @formula.
*/
z3::expr
ast2expr(AST* formula, context & c, const parsingContext & pc);



/* @brief:  Change every variable <name> in an AST to <#name>, and enrich a 
            given parsingContext with this new variables and their
            corresponding types.
   @ast:    The AST member to be modified.
   @pc:     The parsingContext member to be enriched.
   @module: The name of the module where to interpret @ast. 

*/
void
variable_duplicate(AST* ast);


}//namespace parser



#endif // IOSA_COMPLIANCE_H
