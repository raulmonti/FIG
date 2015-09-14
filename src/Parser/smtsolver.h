#ifndef SMT_SOLVER_H
#define SMT_SOLVER_H


#include <z3++.h>
#include "ast.h"
#include "parsingContext.h"


using namespace std;
using namespace z3;

namespace parser{

class SmtSolver{

public:

    SmtSolver(void);

    virtual ~SmtSolver(void);

    /* @sat: check for satisfiability of @formula.
    */
    bool sat (AST *formula, string module, parsingContext & pc);

    /* @sat: check for satisfiability of the conjunction of formulas in @list.
    */
    bool sat (vector<AST*> list, string module, parsingContext & pc);

private:

    /* @ast2expr
    */
    expr ast2expr( AST* formula, string module
                 , context & c, parsingContext & pc);


};

} // Parser

#endif // SMT_SOLVER_H
