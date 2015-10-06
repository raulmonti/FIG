#ifndef SMT_SOLVER_H
#define SMT_SOLVER_H


#include <z3++.h>
#include "ast.h"
#include "parsingContext.h"


using namespace std;
using namespace z3;


namespace parser{


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

    expr build_z3_expr(context & c, string module, parsingContext & pc);
};



/* @sat: check for satisfiability of @formula.
*/
bool sat (AST *formula, string module, parsingContext & pc);

/* @sat: check for satisfiability of the conjunction of formulas in @list.
*/
bool sat (vector<AST*> list, string module, parsingContext & pc);

/* @ast2expr
*/
expr ast2expr( AST* formula, string module
             , context & c, parsingContext & pc);

} // Parser

#endif // SMT_SOLVER_H
