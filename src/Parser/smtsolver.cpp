#include <z3++.h>
#include "parser.h"
#include "smtsolver.h"
#include "ast.h"

using namespace std;
using namespace z3;

namespace parser{



////////////////////////////////////////////////////////////////////////////////
// MODULE API IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

/** 
 * @brief:   Return a z3 expression corresponding to a boolean formula
 *           represented in an AST member. Correctly fill in the z3::context 
 *           member, in order to be able to sat-check over the resulting
 *           expression afterwards.
 * @formula: The AST member to be translated into a z3::expr.
 * @c:       The context member to be filled up.
 * @pc:      A parsingContext member from which to take type information for
 *           each variable in @formula.
 */
//FIXME Use exceptions here instead of assert and cout
expr
ast2expr( AST* formula, context & c, parsingContext & pc){

    //TODO: assert("check that formula is boolean.");
    expr result = c.bool_val(true);

    int bsize = formula->branches.size();
    if( bsize == 3){
        AST *b0 = formula->branches[0];
        AST *b1 = formula->branches[1];
        AST *b2 = formula->branches[2];

        if( b0->tkn == _SEPARATOR ){
            assert(b2->tkn == _SEPARATOR);
            result = ast2expr(b1,c,pc);
        }else{
            expr e0 = ast2expr(b0,c,pc);
            expr e2 = ast2expr(b2,c,pc);
            if(b1->lxm == "+")  result = e0 + e2;
            else if (b1->lxm == "-") result = e0 - e2;
            else if (b1->lxm == "*") result = e0 * e2;
            else if (b1->lxm == "/") result = e0 / e2;
            else if (b1->lxm == "||") result = e0 || e2;
            else if (b1->lxm == "&&") result = e0 && e2;
            else if (b1->lxm == ">") result = e0 > e2;
            else if (b1->lxm == "<") result = e0 < e2;
            else if (b1->lxm == ">=") result = e0 >= e2;
            else if (b1->lxm == "<=") result = e0 <= e2;
            else if (b1->lxm == "==") result = e0 == e2;
            // FIXME Assignments do not correspond to boolean formulas
            // but this next line helps a lot anyway.
            else if (b1->lxm == "=") result = e0 == e2;
            else if (b1->lxm == "!=") result = e0 != e2;
            else {
                cout << b1->lxm << endl;
                assert("Wrong symbol!!!\n" && false);
            }
        }
    }else if (bsize == 2){
        assert(formula->tkn == _VALUE);
        AST *b0 = formula->branches[0];
        AST *b1 = formula->branches[1];
        if(b0->tkn == _NEGATION){
            result = (! ast2expr(b1,c,pc));
        }else if(b0->tkn == _MINUS){
            result = (- ast2expr(b1,c,pc));
        }else{
            assert(false);
        }
    }else if (bsize == 1){
        result = ast2expr(formula->branches[0],c,pc);
    }else if (bsize == 0){
        switch (formula->tkn){
            case _NAME:{
                try{
                    int t = pc[formula->lxm].first;
                    // It is a variable
                    if ( t == T_ARIT){
                        result = c.real_const(formula->lxm.c_str());
                    }else{
                        assert(t == T_BOOL);
                        result = c.bool_const(formula->lxm.c_str());
                    }
                }catch(...){
                    cout << formula->lxm << endl;
                    assert("Nonexistent variable :S" && false);
                }
                break;
            }case _NUM:{
                result = c.real_val(formula->lxm.c_str());
                break;
            }case _BOOLEAN:{
                if(formula->lxm == "true"){
                    result = c.bool_val(true);
                }else if(formula->lxm == "false"){
                    result = c.bool_val(false);
                }else{
                    assert("Wrong boolean value :S" && false);
                }
                break;
            }default:{
                cout << formula->tkn << endl;
                assert("Wrong tkn :S" && false);
            }
        }
    }else{
        assert("Wrong number of branches :S\n" && false);
    }

    return result;

}




/** @brief Change every variable <name> in an AST to <name'>
 *  @ast   The AST member to be modified.
 */
void
variable_duplicate(AST* ast){

    if(ast->tkn == _NAME){
        ast->lxm = ast->lxm+"'";
    }else{
        for(int i = 0; i < ast->branches.size(); ++i){
            variable_duplicate( ast->branches[i]);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////

} //namespace parser

