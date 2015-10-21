#include <z3++.h>
#include "parser.h"
#include "smtsolver.h"
#include "ast.h"
#include "parsingContext.h"

using namespace std;
using namespace z3;

namespace parser{



////////////////////////////////////////////////////////////////////////////////
// SmtFormula CLASS IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

SmtFormula::SmtFormula(SmtFormula *form1, SmtFormula *form2, string optr):
f1(form1),
f2(form2),
op(optr),
ast(NULL)
{}


SmtFormula::SmtFormula(AST* form, string optr):
f1(NULL),
f2(NULL),
op(optr),
ast(form)
{}


SmtFormula::~SmtFormula(){

}


bool
SmtFormula::sat(parsingContext & pc, string module){

    context c;
    solver s(c);

    expr e = build_z3_expr(c, module, pc);

    s.add(e);

    return s.check();
}



expr
SmtFormula::build_z3_expr(context & c, string module, parsingContext & pc){

    expr result = c.bool_val(true);

    if (is_node()){
        if(op == "!"){
            result = ! ast2expr(ast, module, c, pc);
        }else if(op == "-"){
            result = - ast2expr(ast, module, c, pc);
        }else if(op == ""){
            result = ast2expr(ast,module,c,pc);
        }else{
            assert("Not supported unary operator.\n" && false);
        }
    }else{
        expr e0 = f1->build_z3_expr( c, module, pc);
        expr e2 = f2->build_z3_expr( c, module, pc);
        if(op == "+")  result = e0 + e2;
        else if (op == "-") result = e0 - e2;
        else if (op == "*") result = e0 * e2;
        else if (op == "/") result = e0 / e2;
        else if (op == "||") result = e0 || e2;
        else if (op == "&&") result = e0 && e2;
        else if (op == ">") result = e0 > e2;
        else if (op == "<") result = e0 < e2;
        else if (op == ">=") result = e0 >= e2;
        else if (op == "<=") result = e0 <= e2;
        else if (op == "==") result = e0 == e2;
        // FIXME in postconditions we have = instead of == ...
        else if (op == "=") result = e0 == e2; 
        else if (op == "!=") result = e0 != e2;
        else {
            pout << op << endl;
            assert("Not supported binary operator.\n" && false);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////







////////////////////////////////////////////////////////////////////////////////
// MODULE API IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

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
expr
ast2expr( AST* formula, string module
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
/**
bool
sat(AST *formula, string module, parsingContext & pc){

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
**/
/* @sat: check for satisfiability of the conjunction of formulas in @list.
*/


bool 
sat (vector<AST*> list, string module, parsingContext & pc){

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


/**
bool 
check_trans_compat( const AST* g2, const AST *p1, const AST *g1
                  , const parsingContext & pc
                  , const string & module ){

    context c;
    solver s(c);
    expr p1expr = c.bool_val(true);
    expr g1expr = c.bool_val(true);
    expr g2expr = c.bool_val(true);
    AST *mg2 = NULL;
    AST *mg1 = NULL;
    AST *mp1 = NULL;
    parsingContext mpc(pc);


    if(g2){
        mg2 = new AST(g2);
        g2expr = ast2expr(mg2, module, c, mpc);
    }
    if(p1){
        mp1 = new AST(p1);
        vector<AST*> assignList = mp1->get_all_ast(_ASSIG);
        for(int i = 0; i < assignList.size(); ++i){
            assert(assignList[i]->branches.size() == 3);
            variable_duplicate(assignList[i]->branches[0], mpc, module);
            expr assignExpr = 
                ast2expr(assignList[i]->branches[0], module, c, mpc);
            assignExpr = assignExpr == ast2expr( assignList[i]->branches[2]
                                               , module, c, mpc);
            p1expr = p1expr && assignExpr;
        }
    }
    if(g1){
        mg1 = new AST(g1);
        variable_duplicate( mg1, mpc, module);
        g1expr = ast2expr(mg1, module, c, mpc);
    }

    s.add(g2expr && p1expr && g1expr);

    cout << s << endl;

    delete mg1;
    delete mg2; 
    delete mp1;

    return s.check();
}
**/
/**
bool 
sat( vector< AST*> current, vector< AST*> next
   , vector< AST*> assign, const parsingContext & pc
   , const string & moduleName ){

    context c;
    solver s(c);
    expr currentExpr = c.bool_val(true);
    expr assignExpr = c.bool_val(true);
    expr nextExpr = c.bool_val(true);

    parsingContext mpc(pc);


    for(int i = 0; i < current.size(); ++i){
        currentExpr = currentExpr & ast2expr(current[i], moduleName, c, mpc);
    }
    for(int i = 0; i < assign.size(); ++i){
        vector<AST*> assignList = assign[i]->get_all_ast(_ASSIG);
        for(int j = 0; j < assignList.size(); ++j){
            assert(assignList[j]->branches.size() == 3);
            AST *assj = new AST(assignList[j]->branches[0]);
            variable_duplicate( assj, mpc, moduleName);
            expr assjExpr = ast2expr(assj, moduleName, c, mpc);
            assjExpr = assjExpr == ast2expr( assignList[i]->branches[2]
                                               , moduleName, c, mpc);
            assignExpr = assignExpr && assjExpr;
            delete assj;
        }
    }
    
    if(!2next.empty()){

    for(int i = 1; i < next.size(); ++i){
        AST* nexti = new AST(next[i]);
        variable_duplicate( nexti, mpc, moduleName);
        nextExpr = nextExpr && ! ast2expr(nexti, moduleName, c, mpc);
        delete nexti;
    }

    s.add(currentExpr && assignExpr && nextExpr);

    cout << s << endl;


    return s.check();
}
**/


/* @brief:  Change every variable <name> in an AST to <#name>, and enrich a 
            given parsingContext with this new variables and their
            corresponding types.
   @ast:    The AST member to be modified.
   @pc:     The parsingContext member to be enriched.
   @module: The name of the module where to interpret @ast. 

*/
void
variable_duplicate(AST* ast, parsingContext & pc, string module){

    if(ast->tkn == _NAME){
        // update pc
        pc.add_var( module, "#"+ast->lxm, pc.get_var_type( module, ast->lxm));
        // change ast
        ast->lxm = "#"+ast->lxm;
    }else{
        for(int i = 0; i < ast->branches.size(); ++i){
            variable_duplicate( ast->branches[i], pc, module);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////

} //namespace parser

