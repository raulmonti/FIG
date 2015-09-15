#include <z3++.h>
#include "parser.h"
#include "smtsolver.h"
#include "ast.h"
#include "parsingContext.h"

using namespace std;
using namespace z3;


namespace parser{


SmtSolver::SmtSolver(void){}

SmtSolver::~SmtSolver(void){}


/* @ast2expr: return a z3 expression representing the boolean @formula. 
              Correctly fill in the context @c, in order to be able to check
              the result using a solver afterwards.
*/
expr
SmtSolver::ast2expr( AST* formula, string module
                   , context & c, parsingContext & pc){

    //TODO: assert("check that formula is boolean.");
    expr result = c.bool_val(true);

    int bsize = formula->branches.size();
    if( bsize == 3){
        AST *b0 = formula->branches[0];
        AST *b1 = formula->branches[1];
        AST *b2 = formula->branches[2];

        if( b0->tkn == _SEPARATOR ){
            assert(b2->tkn == _SEPARATOR);
            result = ast2expr(b1,module,c,pc);
        }else{
            expr e0 = ast2expr(b0,module,c,pc);
            expr e2 = ast2expr(b2,module,c,pc);
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
            else if (b1->lxm == "=") result = e0 == e2; // FIXME in postconditions we have = instead of == ...
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
            result = (! ast2expr(b1,module,c,pc));
        }else if(b0->tkn == _MINUS){
            result = (- ast2expr(b1,module,c,pc));
        }else{
            assert(false);
        }
    }else if (bsize == 1){
        result = ast2expr(formula->branches[0],module,c,pc);
    }else if (bsize == 0){
        switch (formula->tkn){
            case _NAME:{
                if( pc.has_var(module, formula->lxm)){
                    // It is a variable
                    int t = pc.get_var_type(module, formula->lxm);
                    if ( t == mARIT){
                        result = c.real_const(formula->lxm.c_str());
                    }else{
                        assert(t == mBOOL);
                        result = c.bool_const(formula->lxm.c_str());
                    }
                } else if (pc.has_clock(module, formula->lxm)){
                    // It is a clock                        
                    result = c.real_const(formula->lxm.c_str());
                }else{
                    cout << module << " " << formula->lxm << endl;
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


/* @sat: check for satisfiability of @formula.
   @formula: should be a valid parsed AST, representing a boolean formula, 
             given rules from the @parser module.
   @return: true if satisfiable, false otherwise.
*/

bool
SmtSolver::sat(AST *formula, string module, parsingContext & pc){

    context c;   // from z3
    solver s(c); // from z3

    expr f = ast2expr(formula, module, c, pc);
    s.add(f);

    #ifdef __DEBUG__
    std::cout << "[SMTSOLVER]  Showing smt solver state for checking \n";
    cout << ">>>>>>> " << *formula << endl;
    std::cout << s << "\n";
    std::cout << s.to_smt2() << "\n";
    #endif

    check_result result = s.check();

    #ifdef __DEBUG__
    try{
        cout << s.get_model() << endl;
    }catch(z3::exception ex){
        cout << ex << endl;
    }
    #endif

    return bool(result);
}


/* @sat: check for satisfiability of the conjunction of formulas in @list.
*/
bool 
SmtSolver::sat (vector<AST*> list, string module, parsingContext & pc){

    context c;   // from z3
    solver s(c); // from z3

    for( int i = 0; i < list.size(); ++i){
        expr f = ast2expr(list[i], module, c, pc);
        s.add(f);
    }

    #ifdef __DEBUG__
    std::cout << "[SMTSOLVER]  Showing smt solver state for checking \n";
    cout << ">>>>>>> " << *formula << endl;
    std::cout << s << "\n";
    std::cout << s.to_smt2() << "\n";
    #endif

    check_result result = s.check();

    #ifdef __DEBUG__
    try{
        cout << s.get_model() << endl;
    }catch(z3::exception ex){
        cout << ex << endl;
    }
    #endif

    return bool(result);
}


} //namespace parser

