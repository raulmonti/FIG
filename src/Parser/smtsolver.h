#ifndef SMT_SOLVER_H
#define SMT_SOLVER_H

#include <z3++.h>
#include <map>
#include "ast.h"

using namespace std;
using namespace z3;
namespace parser{

/**
 */
typedef enum    { T_ARIT
                , T_BOOL
                , T_CLOCK
                , T_NOTYPE
                } Type; 
/**
 */
typedef pair<Type,string> ptm; //pair type, module
typedef pair<string,ptm> pvtm; // pair variable, (type,module)
typedef map< string, ptm > parsingContext;





///////////////////////////////////////////////////////////////////////////////
// MODULE API
///////////////////////////////////////////////////////////////////////////////


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
expr ast2expr( AST* formula, context & c, parsingContext & pc);



/* @brief:  Change every variable <name> in an AST to <#name>, and enrich a 
            given parsingContext with this new variables and their
            corresponding types.
   @ast:    The AST member to be modified.
   @pc:     The parsingContext member to be enriched.
   @module: The name of the module where to interpret @ast. 

*/
void
variable_duplicate(AST* ast);

///////////////////////////////////////////////////////////////////////////////


} // Parser


#endif // SMT_SOLVER_H
