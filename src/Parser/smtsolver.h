#ifndef SMT_SOLVER_H
#define SMT_SOLVER_H

#include <z3++.h>
#include "ast.h"
#include "parsingContext.h"



using namespace std;
using namespace z3;
namespace parser{



///////////////////////////////////////////////////////////////////////////////
// CLASS SmtFormula
///////////////////////////////////////////////////////////////////////////////

class SmtFormula{

    SmtFormula *f1;
    SmtFormula *f2;
    string     op;
    AST        *ast;

public:

    SmtFormula(SmtFormula *form1 = NULL, SmtFormula *form2 = NULL,
               string optr = "");

    SmtFormula(AST* form = NULL, string optr = "");

    virtual ~SmtFormula();

    inline bool is_node(){
        return ast != NULL;
    }

    bool sat(parsingContext & pc, string module);

private:

    /* @brief: use this formula to build a z3 expresion representing it.
       @c: a context to fill up while building the formula. To be used by z3
           for sat solving.
    */
    expr build_z3_expr(context & c, string module, parsingContext & pc);
};
///////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////
// MODULE API
///////////////////////////////////////////////////////////////////////////////

/* @sat: check for satisfiability of the conjunction of formulas in @list.
*/
bool sat (vector<AST*> list, string module, parsingContext & pc);


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
expr ast2expr( AST* formula, string module
             , context & c, parsingContext & pc);



/* @brief:  Change every variable <name> in an AST to <#name>, and enrich a 
            given parsingContext with this new variables and their
            corresponding types.
   @ast:    The AST member to be modified.
   @pc:     The parsingContext member to be enriched.
   @module: The name of the module where to interpret @ast. 

*/
void
variable_duplicate(AST* ast, parsingContext & pc, string module);

///////////////////////////////////////////////////////////////////////////////


} // Parser


#endif // SMT_SOLVER_H
