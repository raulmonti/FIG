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
#include "smtsolver.h"
#include "parsingContext.h"


using namespace std;
using namespace parser;


namespace parser{



Verifier::Verifier(parsingContext & pc){
    mPc = & pc;
}


Verifier::~Verifier(void){}



/* @verify: fully verify if @ast compliances with IOSA modeling. Conditions
            (5) and (6) are not ensured here, but should be ensured by the
            simulation engine, i.e. it should provide initial values for
            clocks in every initial state outgoing transitions, and it should
            provide self loops for input actions that are no explicitly
            declared for every state. Condition (4) cant be checked without
            reachability checking. Thus we are only giving out warnings to the
            user and we expect this to be correctly checked by the simulation
            engine and the behavior should be clear to who ever models with our
            simulator.
   @returns: 1 if it compliances, 0 otherwise.
*/
int 
Verifier::verify(AST* ast){

    fill_maps(ast);

    try{
        unique_inputs(ast);     // throws warning string 
    }catch(string warning){
        cout << warning;
    }

    //names_uniqueness(ast);  // throws string error if sthg is not right    
    type_check(ast);        // throws string error if sthg is not right

    input_output_clocks(ast); // 1st and 2nd conditions for IOSA
    unique_outputs(ast);      // 3rd condition for IOSA
    
    check_exhausted_clocks(ast);

    return 1;
}


// FIXME learn to use nicer names for variables!!!!!

/* @names_uniqueness: check that names that should be unique really are.
   @return: 1 if no duplicated name was found.
   @throw: ... if some duplicated name was found.
*/

//FIXME variables and clocks should be called diferent

int
Verifier::names_uniqueness(AST* ast){

    string error_list = "";
    vector<AST*> modules = ast->get_list(_MODULE);
    set<AST*,bool(*)(AST*,AST*)> names 
        ([]( AST* a, AST* b){return a->lxm < b->lxm;});

    /* Unique Modules names. */
    for(int i = 0; i < modules.size();i++){
        AST* a_name = modules[i]->get_first(NAME);
        if(!names.insert(a_name).second){
            AST* duplicated = *names.find(a_name);
            error_list.append(("[ERROR] Duplicated module name '")
                + a_name->p_name() + "', at " + a_name->p_pos() 
                + ". Previously defined at " 
                + duplicated->p_pos() + ".\n");
        }
    }


    set<AST*,bool(*)(AST*,AST*)> cNames 
        ([]( AST* a, AST* b){return a->lxm < b->lxm;}); //clock names.

    /* Unique variables and clock names inside each module. */
    //FIXME or should it be between all modules?
    for(int i = 0; i < modules.size();i++){
        names.clear();  // check uniqueness only inside modules    
        cNames.clear(); // check uniqueness only inside modules    
        AST* var_sec = modules[i]->get_first(_VARSEC);
        if(var_sec){ // Modules have single variable section 
            vector<AST*> vars = var_sec->get_list(_VARIABLE);
            for(int j = 0; j < vars.size(); j++){ // for each clock
                AST* a_name = vars[j]->get_first(_NAME);
                if (!names.insert(a_name).second){
                    AST* duplicated = *names.find(a_name);
                    error_list.append(("[ERROR] Duplicated variable name '")
                        + a_name->p_name() + "', at " + a_name->p_pos() 
                        + ". Previously defined at " 
                        + duplicated->p_pos() + ".\n");
                } 
            } 
        }

        AST* clk_sec = modules[i]->get_first(_CLOCKSEC);
        if(clk_sec){ // Modules have one or none clock sections
            vector<AST*> clks = clk_sec->get_list(_CLOCK);
            for(int j = 0; j < clks.size(); j++){ // for each clock
                AST* a_name = clks[j]->get_first(_NAME);
                if (!cNames.insert(a_name).second){
                    AST* duplicated = *cNames.find(a_name);
                    error_list.append(("[ERROR] Duplicated clock name '")
                        + a_name->p_name() + "', at " + a_name->p_pos() 
                        + ". Previously defined at " 
                        + duplicated->p_pos() + ".\n");
                }
                if(names.count(a_name)){
                    AST* duplicated = *names.find(a_name);
                    error_list.append(("[ERROR] Duplicated variable name '")
                        + a_name->p_name() + "', at " + duplicated->p_pos() 
                        + ". Previous clock with same name defined at " 
                        + a_name->p_pos() + ".\n");
                }
            }
        }
    }

    // Unique property names:
    names.clear();
    vector<AST*> properties = ast->get_all_ast(_PROPERTY);
    for(int i = 0; i < properties.size(); i++){
        AST *name = properties[i]->get_first(_NAME);
        if(!names.insert(name).second){
            AST *duplicated = *names.find(name);
            error_list.append("[ERROR] Duplicated property name '"
                + name->p_name() + "' at " + duplicated->p_pos() + " and "
                + name->p_pos() + ".\n");
        }
    }


    if(error_list != ""){
        throw error_list; // If names are repeated we can not continue ...
    }

    return 1;
}


/* @input_output_clocks: check that input transitions have no clock to wait
                         for. This is in compliance to IOSA first condition.
                         Also check that output transitions have exactly one
                         clock to wait for, in compliance to IOSA second
                         condition.
   @return:
   @throw:
*/
int
Verifier::input_output_clocks(AST* ast){
    // FIXME this will be easier if we force the users to specify ? or !
    // or if we always parse an IO AST even when there is no ? or !
    string error_list = "";
    int result = 1;
    vector<AST*> modules = ast->get_list(_MODULE);
    for(int i = 0; i < modules.size(); i++){
        AST* transSec = modules[i]->get_first(_TRANSEC);
        if(transSec){
            vector<AST*> transitions = 
                transSec->get_list(_TRANSITION);
            for(int i = 0; i < transitions.size(); i++){
                AST* ioAst = transitions[i]->get_first(_IO);
                if(  ioAst 
                  && ioAst->p_name() == "?"
                  && transitions[i]->get_first(_ENABLECLOCK)){
                    error_list.append("[ERROR] In transition declaration at '"
                        + transitions[i]->p_pos() + "'. Input transitions "
                        "should not have to wait for any clocks.\n");
                    result = 0;
                }
                if(  (!ioAst || ioAst->p_name() == "!")
                  && !transitions[i]->get_first(_ENABLECLOCK)){
                    error_list.append("[ERROR] In transition declaration at '"
                        + transitions[i]->p_pos() + "'. Output transitions "
                        "should wait for exactly one clocks.\n");
                    result = 0;
                }
            }
        }
    }

    if(error_list != ""){
        throw error_list; // If names are repeated we can not continue ...
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
    string error_list = "";
    set<AST*,bool(*)(AST*,AST*)> names 
        ([]( AST* a, AST* b){return a->lxm < b->lxm;});

    vector<AST*> modules = ast->get_list(_MODULE);
    for(int i = 0; i < modules.size(); i++){
        names.clear();
        AST* transSec = modules[i]->get_first(_TRANSEC);
        if(transSec){
            vector<AST*> transitions = transSec->get_list(_TRANSITION);
            for(int i = 0; i < transitions.size(); i++){
                AST *enable_c =
                    transitions[i]->get_first(_ENABLECLOCK);
                if(enable_c){
                    if(!names.insert(enable_c).second){
                        AST* duplicated = *names.find(enable_c);
                        error_list.append(("[ERROR] Same clock can not be "
                            "used twice as enable clock. Clock '")
                            + enable_c->p_name() + "', at " 
                            + duplicated->p_pos()
                            + " and " + enable_c->p_pos() + ".\n");
                        result = 0;
                    }
                }
            }
        }
    }
    if (error_list != ""){
        throw error_list;    
    }
    return result;
}

/* @unique_inputs: check property (7) for IOSA.
   @return: 
   @throw: string with errors message if found that the property may not
           be met. (just to take as a warning).
*/
int
Verifier::unique_inputs(AST *ast){

    /*NOTE solving this correctly introduces a reachability problem :S */
    string error_list = "";
    int result = 1;
    set<AST*,bool(*)(AST*,AST*)> names 
        ([]( AST* a, AST* b){return a->lxm < b->lxm;});

    vector<AST*> modules = ast->get_list(_MODULE);
    for(int i = 0; i < modules.size(); i++){
        names.clear();
        AST* transSec = modules[i]->get_first(_TRANSEC);
        if(transSec){
            vector<AST*> transitions = transSec->get_list(_TRANSITION);
            for(int i = 0; i < transitions.size(); i++){
                AST *name = transitions[i]->get_first(_ACTION);
                AST *io = transitions[i]->get_first(_IO);
                if(io && io->p_name() == "?"){
                    if(!names.insert(name).second){
                        AST* duplicated = *names.find(name);
                        error_list.append( "[WARNING] Several input "
                            "transitions with same action can cause non "
                            "determinism if their preconditions are not "
                            "mutually exclusive or their postconditions don't "
                            "describe the same single state. Check transition '"
                            + name->p_name() + "', at " 
                            + duplicated->p_pos()
                            + " and " + name->p_pos() + ".\n");
                        result = 0;
                    }
                }
            }
        }
    }
    if(error_list != ""){
        throw error_list;
    }

    return result;  
}



/* @check_exhausted_clocks: check compliance to condition 4 from IOSA.
   @Note: can only partially check due to reachability issues, thus we 
          report WARNINGS but we don't ensure presence of non-determinism.
*/
int
Verifier::check_exhausted_clocks(AST *ast){
    string error_list = "";
    int result = 1;
    vector<AST*> modules = ast->get_list(_MODULE);
    vector<AST*> satList;

    for (int i = 0 ; i < modules.size() ; i++){
        string module = modules[i]->get_lexeme(_NAME);
        AST* transSec = modules[i]->get_first(_TRANSEC);
        if(transSec){
            vector<AST*> transitions = transSec->get_list(_TRANSITION);
            for(int j = 0; j < transitions.size(); j++){
                AST* t1 = transitions[j];
                AST* io = t1->get_first(_IO);
                string c1 = t1->get_lexeme(_ENABLECLOCK);
                if (c1 != ""){ // only outputs for t1.
                    AST* p1 = t1->get_first(_PRECONDITION);
                    if(p1) satList.push_back(p1); //if p1 != true need to check it
                    for (int k = 0; k < transitions.size(); ++k){
                        // FIXME including t1 ???
                        AST* t2 = transitions[k];
                        vector<string> cv = t2->get_list_lexemes(_RESETCLOCK);
                        set<string> rclks(cv.begin(), cv.end());

                        if (rclks.count(c1) > 0) continue;

                        AST* p2 = t2->get_first(_POSTCONDITION);
                        if(p2){
                            vector<AST*> p2s = p2->get_list(_ASSIG);
                            for(int s=0; s < p2s.size(); ++s){
                                satList.push_back(p2s[s]);
                            }
                        }
                        if(mSolver.sat(satList, module, *mPc)){
                            // t2 is input trans to state where t1 is out trans
                            AST* pp2 = t2->get_first(_PRECONDITION);
                            for(int h = 0; h < transitions.size(); ++h){
                                AST *t3 = transitions[h];
                                string c3 = t3->get_lexeme(_ENABLECLOCK);
                                if(c3 == "") continue; // not an output
                                if(c1 == c3) break;
                                AST* p3 = t3->get_first(_PRECONDITION);
                                satList.clear();
                                if(pp2) satList.push_back(pp2);
                                if(p3) satList.push_back(p3);
                                if(mSolver.sat(satList, module, *mPc)){
                                    error_list.append("WARNING");
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if(error_list != ""){
        throw error_list;
    }
    return 1;
}





/* @fill_maps: fill the context @mPc for @ast.
*/
int
Verifier::fill_maps(AST *ast){

    /*NOTE: names of variables and clock can be accessed outside of its
            modules by using <module name>.<variable/clock name>. In the
            type maps this names will be kept under the special module name 
            '#property'.
    */
    vector<AST*> modules = ast->get_list(_MODULE);
    for(int i = 0; i < modules.size(); i++){
        vector<AST*> variables = modules[i]->get_all_ast(_VARIABLE);
        vector<AST*> clocks = modules[i]->get_all_ast(_CLOCK);
        string module = modules[i]->get_lexeme(_NAME);
        // Fill the variable type maps
        for(int j=0; j < variables.size(); j++){
            string name = variables[j]->get_lexeme(_NAME);
            string type = variables[j]->get_lexeme(_TYPE);
            if(type == "Int" || type == "Float"){
                mPc->add_var(module,name,mARIT);
                mPc->add_var("#property",module+"."+name,mARIT);
            }else if (type == "Bool"){
                mPc->add_var(module,name,mBOOL);
                mPc->add_var("#property",module+"."+name,mBOOL);
            }else{
                assert(false);      // FIXME is it good to do this?
            }
        }
        //Fill clocks map
        for(int j=0; j < clocks.size(); j++){
            string name = clocks[j]->get_lexeme(_NAME);
            mPc->add_clock(module, name);
            mPc->add_clock("#property", module+"."+name);
        }
    }
    return 1;
}

int
Verifier::type_check(AST *ast){

    string error_list = "";
    int result = 1;
    vector<AST*> modules = ast->get_list(_MODULE);
    for(int i = 0; i < modules.size(); i++){
        vector<AST*> variables = modules[i]->get_all_ast(_VARIABLE);
        vector<AST*> clocks = modules[i]->get_all_ast(_CLOCK);
        string module = modules[i]->get_lexeme(_NAME);

        // Type check initializations:
        for(int j=0;j<variables.size();j++){
            string type = variables[j]->get_lexeme(_TYPE);
            AST* init = variables[j]->get_first(_INIT);
            if(init){
                try{
                    Type t = get_type( init->get_first(_EXPRESSION)
                                     , module);
                    if( ((type == "Bool") && t != mBOOL) || 
                        ((type == "Int" || type == "Float") && t != mARIT)){
                        AST* v = variables[j]->get_first(_NAME);
                        throw "[ERROR] Wrong type for initialization of "
                              "variable '" + v->p_name() + "', at " 
                              + v->p_pos() + ".\n";
                    }
                }catch(string s){
                    error_list.append(s);
                }
            }
        }

        // Type check transitions preconditions:
        vector<AST*> trans = modules[i]->get_all_ast(_TRANSITION);
        for(int j =0; j < trans.size();++j){
            AST *expr = trans[j]->get_first(_EXPRESSION);
            if (expr){
                try{
                    if( mBOOL != get_type(expr, module)){
                        throw "[ERRPR] Wrong type for transitions "
                              "precondition at " + expr->p_pos() 
                              + ". It should be boolean but found "
                              "arithmetic instead.\n";
                    }
                }catch(string err){
                    error_list.append(err);
                }
            }
        }
        
        // Type check assignments in post conditions
        for(int j = 0; j < trans.size(); ++j){
            vector<AST*> assigs = trans[j]->get_all_ast(_ASSIG);
            for(int k = 0; k < assigs.size(); ++k){
                AST* var = assigs[k]->get_first(_NAME);
                string vname = var->p_name();
                AST* expr = assigs[k]->get_first(_EXPRESSION);
                try{
                    Type t1 = mPc->get_var_type(module,vname);
                    Type t2 = get_type(expr,module);
                    if(t1 != t2){
                        throw "[ERROR] Wrong type in assignment of variable "
                              + vname + " at " + var->p_pos() + ".\n";
                    }
                }catch(string err){
                    error_list.append(err);
                }catch(out_of_range err){
                    error_list.append( "[ERROR] Undeclared variable " 
                                     + vname + " at " + var->p_pos() 
                                     + ".\n");
                }
            }
        }

        // Check that enabling clock and reseting clocks are really clocks
        for(int j = 0; j < trans.size(); j++){
            AST* enable = trans[j]->get_first(_ENABLECLOCK);
            if(enable && !mPc->has_clock(module,enable->p_name())){
                error_list.append( "[ERROR] No clock named " 
                                 + enable->p_name() + " at " 
                                 + enable->p_pos() + ".\n" );
            }
            vector<AST*> resets = trans[j]->get_all_ast(_RESETCLOCK);
            for(int k = 0; k < resets.size(); ++k){
                if(!mPc->has_clock(module,resets[k]->p_name())){
                    error_list.append( "[ERROR] No clock named " 
                                     + resets[k]->p_name() + " at " 
                                     + resets[k]->p_pos() + ".\n" );
                }
            }
        }
    }

    // Type check properties:
    vector<AST*> properties = ast->get_all_ast(_PROPERTY);
    for(int i = 0; i < properties.size(); i++){
        AST *exp = properties[i]->get_first(_EXPRESSION);
        try{
            if(mBOOL != get_type(exp, "#property")){
                throw "[ERROR] Found non boolean expression inside property, "
                      "at " + exp->p_pos() + ".\n";
            }
        }catch (string err){
            error_list.append(err);
        }    
    }

    if(error_list != ""){
        throw error_list; // Need to solve typing to continue.
    }

    return result;
}





/* @get_type: Return type of an expression.
   @return: Type of the expression if it has one.
   @throw: string with error message if there is something wrong with typing.
*/
Type
Verifier::get_type(AST *expr, string module){

    assert(expr);
    if(expr->tkn == _EXPRESSION){
        AST *equality = expr->get_branch(0);
        Type t1 = get_type(equality, module);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2, module);
            if(t1 != mBOOL || t1 != t2){
                throw "[ERROR] Wrong types for binary operator "
                      "at " + op->p_pos() + ".\n";
            }else{
                return mBOOL; 
            }
        }
        return t1;
    }else if(expr->tkn == _EQUALITY){
        AST *comparison = expr->get_branch(0);
        Type t1 = get_type(comparison,module);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,module);
            if(t1 != t2){
                throw "[ERROR] Wrong types for equality "
                      "operator at " + op->p_pos() + ".\n";
            }else{
                return mBOOL; 
            }
        }
        return t1;
    }else if(expr->tkn == _COMPARISON){
        AST *summation = expr->get_branch(0);
        Type t1 = get_type(summation,module);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,module);
            if(t1 != mARIT || t1 != t2){
                throw "[ERROR] Wrong types for arithmetic "
                      "comparison at " + op->p_pos() + ".\n";
            }else{
                return mBOOL; 
            }
        }
        return t1;
    }else if(expr->tkn == _SUM){
        AST *division = expr->get_branch(0);
        Type t1 = get_type(division,module);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,module);
            if(t1 != mARIT || t1 != t2){
                throw "[ERROR] Wrong types for arithmetic "
                      "operation at " + op->p_pos() + ".\n";
            }else{
                return mARIT; 
            }
        }
        return t1;
    }else if(expr->tkn == _DIV){
        AST *value = expr->get_branch(0);
        Type t1 = get_type(value,module);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,module);
            // FIXME should check division by zero????
            if(t1 != mARIT || t1 != t2){
                throw "[ERROR] Wrong types for arithmetic "
                      "operation at " + op->p_pos() + ".\n";
            }else{
                return mARIT; 
            }
        }
        return t1;
    }else if(expr->tkn == _VALUE){
        AST *value = expr->get_branch(0);
        Type t;
        switch (value->tkn){
            case _NAME:
                // FIXME if variable is not declared...
                if (mPc->has_var(module, value->lxm)){
                    t = mPc->get_var_type(module,value->lxm);
                }else if(mPc->has_clock(module,value->lxm)){
                    t = mARIT;
                }else{
                    throw "[ERROR] Undeclared variable '" + value->lxm 
                          + "' at " + value->p_pos() + ".\n";
                }
                break;
            case _BOOLEAN:
                t = mBOOL;
                break;
            case _NUM:
                t = mARIT;
                break;
            case _SEPARATOR:
                t = get_type(expr->get_branch(1),module);
                break;
            case _NEGATION:
                t = get_type(expr->get_branch(1),module);
                if(t != mBOOL){
                    throw "[ERROR] Wrong type for boolean negation, at " 
                          + value->p_pos() + ".\n";
                }
                break;
            case _MINUS:
                t = get_type(expr->get_branch(1),module);
                if(t != mARIT){
                    throw "[ERROR] Wrong type for arithmetic negation, at " 
                          + value->p_pos() + ".\n";
                }
                break;
        }
        return t;
    }else{
        assert(0);
    }
    assert(0);
}

} // namespace parser
