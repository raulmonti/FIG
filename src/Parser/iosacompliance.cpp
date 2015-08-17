/**

    IOSA compliance module for FIG
    Raul Monti
    2015

**/

#include <set>
#include <assert.h>
#include "parser.h"
#include "iosacompliance.h"
#include "ast.h"
#include "exceptions.h"


using namespace std;


/* @verify: fully verify if @ast compliances with IOSA modeling.
   @returns: 1 if it compliances, 0 otherwise.
*/
int 
Verifier::verify(AST* ast){

    error_list = "";
    names_uniqueness(ast);
    input_output_clocks(ast); // 1st and 2nd conditions for IOSA
    unique_outputs(ast);      // 3rd condition for IOSA
    type_check(ast);

    if(error_list != ""){
        throw error_list;
    }
    return 1;
}


// FIXME learn to use nicer names for variables!!!!!

/* @names_uniqueness: check that names that should be unique really are.
   @return: 1 if no duplicated name was found.
   @throw: ... if some duplicated name was found.
*/

int
Verifier::names_uniqueness(AST* ast){

    vector<AST*> modules = ast->get_list(parser::_MODULE);
    set<AST*,bool(*)(AST*,AST*)> names 
        ([]( AST* a, AST* b){return a->lxm < b->lxm;});

    /* Unique Modules names. */
    for(int i = 0; i < modules.size();i++){
        AST* a_name = modules[i]->get_first(parser::NAME);
        if(!names.insert(a_name).second){
            AST* duplicated = *names.find(a_name);
            error_list.append(("[ERROR] Duplicated module name '")
                + a_name->p_name() + "', at " + a_name->p_pos() 
                + ". Previously defined at " 
                + duplicated->p_pos() + ".\n");
        }
    }


    /* Unique variables names inside each module. */
    //FIXME or should it be between all modules
    for(int i = 0; i < modules.size();i++){
        names.clear(); // check uniquenes only inside modules    
        AST* var_sec = modules[i]->get_first(parser::_VARSEC);
        if(var_sec){ // Modules have single variable section 
            vector<AST*> vars = var_sec->get_list(parser::_VARIABLE);
            for(int j = 0; j < vars.size(); j++){ // for each clock
                AST* a_name = vars[j]->get_first(parser::_NAME);
                if (!names.insert(a_name).second){
                    AST* duplicated = *names.find(a_name);
                    error_list.append(("[ERROR] Duplicated variable name '")
                        + a_name->p_name() + "', at " + a_name->p_pos() 
                        + ". Previously defined at " 
                        + duplicated->p_pos() + ".\n");
                } 
            } 
        }
    }

    /* Unique clock names between all modules. */
    //FIXME or should it be inside each module
    names.clear();
    for(int i = 0; i < modules.size();i++){ // For each module
        AST* clk_sec = modules[i]->get_first(parser::_CLOCKSEC);
        if(clk_sec){ // Modules have one or none clock sections
            vector<AST*> clks = clk_sec->get_list(parser::_CLOCK);
            for(int j = 0; j < clks.size(); j++){ // for each clock
                AST* a_name = clks[j]->get_first(parser::_NAME);
                if (!names.insert(a_name).second){
                    AST* duplicated = *names.find(a_name);
                    error_list.append(("[ERROR] Duplicated clock name '")
                        + a_name->p_name() + "', at " + a_name->p_pos() 
                        + ". Previously defined at " 
                        + duplicated->p_pos() + ".\n");
                } 
            }
        }
    }
}


/* @input_output_clocks: check that input transitions have no clock to wait
                         for. This is in compilance to IOSA first condition.
                         Also check that output transitions have exactly one
                         clock to wait for, in compilance to IOSA second
                         condition.
   @return:
   @throw:
*/
int
Verifier::input_output_clocks(AST* ast){
    // FIXME this will be easier if we force the users to specify ? or !
    // or if we always parse an IO AST even when there is no ? or !
    int result = 1;
    vector<AST*> modules = ast->get_list(parser::_MODULE);
    for(int i = 0; i < modules.size(); i++){
        AST* transSec = modules[i]->get_first(parser::_TRANSEC);
        if(transSec){
            vector<AST*> transitions = transSec->get_list(parser::_TRANSITION);
            for(int i = 0; i < transitions.size(); i++){
                AST* ioAst = transitions[i]->get_first(parser::_IO);
                if(  ioAst 
                  && ioAst->p_name() == "?"
                  && transitions[i]->get_first(parser::_ENABLECLOCK)){
                    error_list.append("[ERROR] In transition declaration at '"
                        + transitions[i]->p_pos() + "'. Input transitions "
                        "should not have to wait for any clocks.\n");
                    result = 0;
                }
                if(  (!ioAst || ioAst->p_name() == "!")
                  && !transitions[i]->get_first(parser::_ENABLECLOCK)){
                    error_list.append("[ERROR] In transition declaration at '"
                        + transitions[i]->p_pos() + "'. Output transitions "
                        "should wait for exactly one clocks.\n");
                    result = 0;
                }
            }
        }
    }
    return result;
}

/* FIXME for the 3rd condition for IOSA we can take three different approaches:
     1) we don't allow clocks to appear more than once in a transition
        precondition (easy). Only cons is that we can't reuse clocks and thus
        we loose efficiency. 
     2) We check that if the clocks are the same then the state preconditions
        are mutually exclusive (different origin states) or that a state
        in the intersection set is unreachable (difficult).
     3) We unify the output transitions with same clocks (should have same
        actions too and may have unfeasible preconditions after unification).
   We go for the easier first (1). This is the most logical one to use anyway.
*/

/* @unique_ouputs: check that clocks are used only once as transition enable
                   clocks, in compliance to 3rd condition for IOSA.
   @throw:
   @result:
*/
int
Verifier::unique_outputs(AST *ast){
    /* FIXME some things like the set names and vector modules are used a lot
             and could be defined and built at the constructor or at @verify
             method avoiding code repetition and improving efficiency. 
    */
    int result = 1;
    set<AST*,bool(*)(AST*,AST*)> names 
        ([]( AST* a, AST* b){return a->lxm < b->lxm;});

    vector<AST*> modules = ast->get_list(parser::_MODULE);
    for(int i = 0; i < modules.size(); i++){
        names.clear();
        AST* transSec = modules[i]->get_first(parser::_TRANSEC);
        if(transSec){
            vector<AST*> transitions = transSec->get_list(parser::_TRANSITION);
            for(int i = 0; i < transitions.size(); i++){
                AST *enable_c =
                    transitions[i]->get_first(parser::_ENABLECLOCK);
                if(enable_c){
                    AST *c = enable_c->get_first(parser::_NAME);
                    if(!names.insert(c).second){
                        AST* duplicated = *names.find(c);
                        error_list.append(("[ERROR] Same clock can not be "
                            "used twice as enable clock. Clock '")
                            + c->p_name() + "', at " + duplicated->p_pos()
                            + " and " + c->p_pos() + ".\n");
                        result = 0;
                    }
                }
            }
        }
    }
    return result;
}


int
Verifier::type_check(AST *ast){

    int result = 1;
    vector<AST*> modules = ast->get_list(parser::_MODULE);
    for(int i = 0; i < modules.size(); i++){
        vector<AST*> variables = modules[i]->get_all_ast(parser::_VARIABLE);
        vector<AST*> clocks = modules[i]->get_all_ast(parser::_CLOCK);
        vector<AST*> expressions =
            modules[i]->get_all_ast(parser::_EXPRESSION);
        string module = modules[i]->get_lexeme(parser::_NAME);

        // Fill the variable type maps
        for(int j=0; j < variables.size(); j++){
            string name = variables[j]->get_lexeme(parser::_NAME);
            string type = variables[j]->get_lexeme(parser::_TYPE);
            if(type == "int"){
                typeMap[module][name] = mARIT;
            }else{
                typeMap[module][name] = mBOOL;
            }
        }

        for(int j= 0; j < expressions.size(); j++){
            cout << "type: " << get_type(expressions[j], module) << endl;
        }
    }

    return result;
}





/* @get_type: Return type of an expression.
   @return:
   @throw:
*/
Verifier::Type
Verifier::get_type(AST *expr, string module){
    //FIXME verify that I'm not copying maps but passing references.

    assert(expr);
    if(expr->tkn == parser::_EXPRESSION){
        AST *equality = expr->get_branch(0);
        Type t1 = get_type(equality, module);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2, module);
            if(t1 != mBOOL || t1 != t2){
                error_list.append( "Wrong types for binary operator at " 
                                 + op->p_pos() + ".\n");
            }else{
                return mBOOL; 
            }
        }
        return t1;
    }else if(expr->tkn == parser::_EQUALITY){
        AST *comparison = expr->get_branch(0);
        Type t1 = get_type(comparison,module);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,module);
            if(t1 != t2){
                error_list.append( "Wrong types for equality operator at " 
                                 + op->p_pos() + ".\n");
            }else{
                return mBOOL; 
            }
        }
        return t1;
    }else if(expr->tkn == parser::_COMPARISON){
        AST *summation = expr->get_branch(0);
        Type t1 = get_type(summation,module);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,module);
            if(t1 != mARIT || t1 != t2){
                error_list.append( "Wrong types for arithmetic comparison at " 
                                 + op->p_pos() + ".\n");
            }else{
                return mBOOL; 
            }
        }
        return t1;
    }else if(expr->tkn == parser::_SUM){
        AST *division = expr->get_branch(0);
        Type t1 = get_type(division,module);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,module);
            if(t1 != mARIT || t1 != t2){
                error_list.append( "Wrong types for arithmetic operation at " 
                                 + op->p_pos() + ".\n");
            }else{
                return mARIT; 
            }
        }
        return t1;
    }else if(expr->tkn == parser::_DIV){
        AST *value = expr->get_branch(0);
        Type t1 = get_type(value,module);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,module);
            // FIXME should check division by zero????
            if(t1 != mARIT || t1 != t2){
                error_list.append( "Wrong types for arithmetic operation at " 
                                 + op->p_pos() + ".\n");
            }else{
                return mARIT; 
            }
        }
        return t1;
    }else if(expr->tkn == parser::_VALUE){
        AST *value = expr->get_branch(0);
        Type t;
        switch (value->tkn){
            case parser::_NAME:
                // FIXME if variable is not declared...
                t = (*(typeMap[module].find(value->lxm))).second;
                break;
            case parser::_BOOLEAN:
                t = mBOOL;
                break;
            case parser::_NUM:
                t = mARIT;
                break;
            case parser::_SEPARATOR:
                t = get_type(expr->get_branch(1),module);
                break;
            case parser::_NEGATION:
                t = get_type(expr->get_branch(1),module);
                if(t != mBOOL){
                    error_list.append( "Wrong type for negation, at " 
                                     + value->p_pos() + ".\n");
                    //t = -1; FIXME change for NOTYPE and do the same for the
                    // rest of the cases where the typing is wrong.
                }
                break;
        }
        return t;
    }else{
        assert(0);
    }
    assert(0);
}

