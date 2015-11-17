/**

    IOSA compliance module for FIG
    Raul Monti
    2015

**/

#include <set>
#include <assert.h>
#include <exception>
#include <tuple>
#include <vector>
#include <set>
#include <algorithm>
#include "parser.h"
#include "iosacompliance.h"
#include "ast.h"
#include "exceptions.h"
#include "smtsolver.h"
#include "parsingContext.h"



/*FIXME decide if the following definition is useful and do the same with
        the rest of long error messages if so.
*/

#define W_0(l1,l2) "[WARNING] Nondeterminism may be present if we reach states"\
                   " where transitions at lines " +  l1 + " and " + l2 + \
                   " are enabled, since they use the same clock and they" \
                   " may reach different states. Check IOSA condition 3.\n"


using namespace std;

namespace parser{


////////////////////////////////////////////////////////////////////////////////
// USEFUL FUNCTIONS
////////////////////////////////////////////////////////////////////////////////


/* @brief: check if the transition parsed into an AST corresponds to an
           output transition.
*/
bool
is_output(AST* trans){

    bool result = true;
    string direction = trans->get_lexeme(_IO);
    if(direction == "?"){
        result = false;
    }
    return result;
}

/* @brief: check if @transition1 enabling clock is not reseted by @transition2.

*/
bool
clock_nreset(AST* trans1, AST* trans2){

    string enableCName = "";
    try{
        enableCName = trans1->get_first(_ENABLECLOCK)->get_lexeme(_NAME);
    }catch(std::exception & e){
        throw string("Couldn't find clock\n") + e.what();
    }

    vector<string> resetCList = trans2->get_list_lexemes(_RESETCLOCK);
    for(int i = 0; i < resetCList.size(); i++){
        if(enableCName == resetCList[i]) return false;
    }

    return true;
}


/* @brief: find out if both transitions have the same enabling clock, if they
           have any.
*/
bool
same_clock(AST* trans1, AST* trans2){

    string enableC1Name = "";
    string enableC2Name = "";

    AST* enableC1 = trans1->get_first(_ENABLECLOCK);
    if(enableC1){
        enableC1Name = enableC1->get_lexeme(_NAME);
    }
    AST* enableC2 = trans2->get_first(_ENABLECLOCK);
    if(enableC2){
        enableC2Name = enableC2->get_lexeme(_NAME);
    }
    if(enableC1Name != "" && enableC1Name == enableC2Name){
        return true;
    }else{
        return false;
    }

}


/* @brief: check if two transitions reset the same set of clocks.
*/
bool
same_rclocks(AST* t1, AST* t2){

    vector<string> rc1 = t1->get_all_lexemes(_RESETCLOCK);
    vector<string> rc2 = t2->get_all_lexemes(_RESETCLOCK);
    std::sort(rc1.begin(), rc1.end());
    std::sort(rc2.begin(), rc2.end());
    return  rc1 == rc2;
}



/* @brief: check if transitions produce the same action.
*/
bool
same_action(AST* t1, AST* t2){

    return t1->get_lexeme(_ACTION) == t2->get_lexeme(_ACTION);

}


/* @brief:  interpret a postcondition formula and build the corresponding z3
            expression.
   @return:  

*/
z3::expr
post2expr(AST* pAst, string mname, parsingContext &pc, z3::context &c){

    z3::expr pExp = c.bool_val(true);
    set<string> vNameSet;

    // add the valuations given by the postcondition
    vector<AST*> pList = pAst->get_all_ast(_ASSIG);
    for(int k = 0; k < pList.size(); ++k){
        AST* var = new AST(pList[k]->branches[0]);
        AST* val = pList[k]->branches[2];
        vNameSet.insert(var->get_lexeme(_NAME));
        variable_duplicate(var,pc,mname);
        pExp = pExp && (ast2expr(var,mname,c,pc) == ast2expr(val,mname,c,pc));
    }
    // add the remaining conditions (unchanged ones)
    vector<pair<string,Type>> vlist = pc.get_type_list(mname);
    for(auto const &it : vlist){
        string vname = it.first;
        if(0 >= vNameSet.count(vname) && vname[0] != '#'){
            if(it.second == mARIT){
                z3::expr vexpr = c.real_const(string('#'+vname).c_str());
                pExp = pExp && (vexpr == c.real_const(vname.c_str()));
            }else if(it.second == mBOOL){
                z3::expr vexpr = c.bool_const(string('#'+vname).c_str());
                pExp = pExp && (vexpr == c.bool_const(vname.c_str()));
            }else{
                assert(false);
            }
        }
    }

    return pExp;
}




////////////////////////////////////////////////////////////////////////////////
// VERIFIER CLASS IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

Verifier::Verifier(parsingContext & pc){
    mPc = & pc;
}


Verifier::~Verifier(void){}


////////////////////////////////////////////////////////////////////////////////

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

    // fill up the parsing context.
    // Also checks for the correct declaration of variables.
    fill_maps(ast);

    try{
        pout << ">> Check names uniqueness...\n";
        names_uniqueness(ast);    // throws string error if sthg is not right    
        pout << ">> Check typing...\n";
        type_check(ast);          // throws string error if sthg is not right

        pout << ">> Check 1st and 2nd IOSA conditions...\n";
        input_output_clocks(ast); // 1st and 1nd conditions for IOSA
        pout << ">> Check 3rd IOSA condition...\n";
        unique_outputs(ast);      // 3rd condition for IOSA        
        pout << ">> Check 4th IOSA condition...\n";
        check_exhausted_clocks(ast);
        pout << ">> Check 7th IOSA condition...\n";
        check_input_determinism(ast);

    }catch(string warning){
        cout << warning;
        return 0;
    }

    return 1;
}



////////////////////////////////////////////////////////////////////////////////

/* @brief:  check that names that should be unique really are.
   @return: 1 if no duplicated name was found.
   @throw:  string if some duplicated name was found.
*/
int
Verifier::names_uniqueness(AST* ast){

    string error_list = "";
    vector<AST*> modules = ast->get_list(_MODULE);
    set<AST*,bool(*)(AST*,AST*)> names 
        ([]( AST* a, AST* b){return a->lxm < b->lxm;});

    /* Unique Modules names. */
    for(int i = 0; i < modules.size(); ++i){
        AST* a_name = modules[i]->get_first(_NAME);
        if(!names.insert(a_name).second){
            AST* duplicated = *names.find(a_name);
            error_list.append(("[ERROR] Duplicated module name '")
                + a_name->p_name() + "', at " + a_name->p_pos() 
                + ". Previously defined at " 
                + duplicated->p_pos() + ".\n");
        }
    }
    /* Unique variables and clock names inside each module. */
    set<AST*,bool(*)(AST*,AST*)> cNames 
        ([]( AST* a, AST* b){return a->lxm < b->lxm;}); //clock names.
    for(int i = 0; i < modules.size();i++){
        names.clear();  // check uniqueness only inside modules    
        cNames.clear();
        vector<AST*> vars = modules[i]->get_all_ast(_VARIABLE);
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
        vector<AST*> clks = modules[i]->get_all_ast(_CLOCK);
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
        throw error_list; // If names are repeated we can not continue
    }
    return 1;
}


////////////////////////////////////////////////////////////////////////////////

/* @brief:  check that input transitions have no clock to wait
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
                if(ioAst && ioAst->p_name() == "?"
                  && transitions[i]->get_first(_ENABLECLOCK)){
                    error_list.append("[ERROR] In transition declaration at '"
                        + transitions[i]->get_pos() + "'. Input transitions "
                        "should not have to wait for any clocks.\n");
                    result = 0;
                }
                if(  (!ioAst || ioAst->p_name() == "!")
                  && !transitions[i]->get_first(_ENABLECLOCK)){
                    error_list.append("[ERROR] In transition declaration at '"
                        + transitions[i]->get_pos() + 
                        "'. Output transitions should wait for exactly one "
                        "clock.\n");
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


////////////////////////////////////////////////////////////////////////////////

/* @brief: check IOSA condition 3. (without reachability check)
   @throw:
   @result:

   @algorithm: if(  (c1 == c2) 
                 && sat(pre1 && pre2)
                 && (a1 != a2 || C'1 != C'2 || s1 != s2) ) : WARNING
*/

int
Verifier::unique_outputs(AST *ast){
    /* FIXME some things like the set names and vector modules are used a lot
             and could be defined and built at the constructor or at @verify
             method avoiding code repetition and improving efficiency. 
    */

    int result = 1;
    string error_list = "";
    z3::context c;
    z3::solver s(c);
    parsingContext pc(*mPc); // copy the context    

    vector<AST*> modules = ast->get_list(_MODULE);
    for(int k = 0; k < modules.size(); ++k){
        string module = modules[k]->get_lexeme(_NAME);
        vector<AST*> transs = modules[k]->get_all_ast(_TRANSITION);
        for(int i = 0; i < transs.size(); i++){
            for(int j = i+1; j < transs.size(); j++){
                if( same_clock(transs[i], transs[j])){
                    AST* pre1 = transs[i]->get_first(_PRECONDITION);
                    AST* pre2 = transs[j]->get_first(_PRECONDITION);
                    if(pre1) pre1 = pre1->get_first(_EXPRESSION);
                    if(pre2) pre2 = pre2->get_first(_EXPRESSION);
                    z3::expr e1 = ast2expr(pre1, module, c, pc);                            
                    z3::expr e2 = ast2expr(pre2, module, c, pc);
                    s.reset();
                    s.add(e1 && e2);

                    if( s.check() ){
                        string line1 = transs[i]->get_line();
                        string line2 = transs[j]->get_line();
                        if(!same_action(transs[i], transs[j])){
                            error_list.append("[WARNING] Nondeterminism"
                                " may be present if we reach states"
                                " where transitions at lines " +  line1
                                + " and " + line2 + " are enabled, "
                                "since they use the same clock and they"
                                " produce different actions. Check "
                                " IOSA condition 3.\n");
                        }
                        if(!same_rclocks(transs[i], transs[j])){
                            error_list.append("[WARNING] Nondeterminism"
                                " may be present if we reach states"
                                " where transitions at lines " +  line1
                                + " and " + line2 + " are enabled, "
                                "since they use the same clock and they"
                                " reset different clocks. Check "
                                " IOSA condition 3.\n");
                        }
                        z3::expr p1 = post2expr(transs[i],module,pc,c);
                        z3::expr p2 = post2expr(transs[j],module,pc,c);
                        s.reset();
                        s.add(e1 && e2 && (p1 != p2));
                        if(s.check()){
                            error_list.append(W_0(line1,line2));
                        }
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


/* @check_exhausted_clocks: check compliance to condition 4 from IOSA.
   @Note: can only partially check due to reachability issues, thus we 
          report WARNINGS but we don't ensure presence of non-determinism.
*/
int
Verifier::check_exhausted_clocks(AST *ast){

    string          error_list  = "";
    int             result      = 1;
    parsingContext  pc(*mPc);                        // a copy of mPc
    z3::context     c;
    z3::solver      s(c);
    z3::expr        ex          = c.bool_val(true);

    vector<AST*> mods = ast->get_list(_MODULE);
    for (int i = 0 ; i < mods.size() ; i++){
        string mname = mods[i]->get_lexeme(_NAME);
        vector<AST*> transs = mods[i]->get_all_ast(_TRANSITION);
        for(int i = 0; i < transs.size(); i++){
            if(is_output(transs[i])){
                for(int j = 0; j < transs.size(); ++j){
                    // check that i and j are different transitions and that 
                    // j does not reset the enabling clock from i.
                    if(i != j && clock_nreset(transs[i],transs[j])){
                        AST *pre = transs[i]->get_first(_PRECONDITION);
                        if(pre){
                            AST *g1 = new AST(pre);
                            // g1 is evaluated after the transition
                            variable_duplicate(g1,pc,mname);
                            ex = ast2expr(g1, mname, c, pc);
                        }
                        AST *p2 = transs[j]->get_first(_POSTCONDITION);
                        if(p2){

                            vector<AST*> assigns = p2->get_list(_ASSIG);
                            for(int k = 0; k < assigns.size(); ++k){
                                AST* var = new AST(assigns[k]->branches[0]);
                                // the assignments are over next variables.
                                variable_duplicate(var,pc,mname);
                                AST* val = assigns[k]->branches[2];
                                z3::expr e1 = ast2expr(var,mname,c,pc);
                                z3::expr e2 = ast2expr(val,mname,c,pc);
                                ex = ex && ( e1 == e2 );
                            }
                        }
                        AST * g2 = transs[j]->get_first(_PRECONDITION);
                        if(g2){
                            ex = ex && ast2expr(g2,mname,c,pc);
                        }
                        for(int k = 0; k < transs.size(); ++k){
                            // !g for guards of every output transitions 
                            // going out from the same state as j
                            // and same clock as i.
                            if(j != k && same_clock(transs[i], transs[k])){
                                AST *g = transs[k]->get_first(_PRECONDITION);
                                if(g){
                                    ex = ex && (! ast2expr(g,mname,c,pc));
                                }
                            }
                        }
                        s.add(ex);
                        if(s.check()){
                            string namei = transs[i]->get_lexeme(_ACTION);
                            string namej = transs[j]->get_lexeme(_ACTION);
                            string linei = transs[i]->get_line();
                            string linej = transs[j]->get_line();
                            error_list.append("[WARNING] it is possible that"
                                   " transition " + namei
                                 + " at line " + linei + " could be reached "
                                 + "with its clock exhausted via transition " 
                                 + namej + " at line " + linej + ", possibly " 
                                 + "causing non determinisim. Check IOSA "
                                 + "condition 4.\n");
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



////////////////////////////////////////////////////////////////////////////////

/* Check condition 7 for IOSA.

    if (act1 = act2):
        if (sat ( pre1(x) && pre2(x)  // there is a valuation for both pre 
               && (pos1(x,x') != pos2(x,x')) // that makes both post different
                ) 
           ):
            WARNING.
*/

int
Verifier::check_input_determinism(AST *ast){

    string error_list = "";
    int result = 1;
    parsingContext pc(*mPc);
    vector<AST*> modules = ast->get_list(_MODULE);
    z3::context c;
    z3::expr e = c.bool_val(true);
    z3::solver s(c);

    for(int m = 0; m < modules.size(); ++m){
        AST* mod = modules[m];
        string module = mod->get_lexeme(_NAME);
        vector<AST*> inputTrans;
        vector<AST*> trans = mod->get_all_ast(_TRANSITION);
        for(int t = 0; t < trans.size(); ++t){
            if(!is_output(trans[t])){
                inputTrans.push_back(trans[t]);
            }
        }
        for(int i = 0; i < inputTrans.size(); ++i){
            for(int j = i+1; j < inputTrans.size(); ++j){
                string ti = inputTrans[i]->get_lexeme(_ACTION);
                string tj = inputTrans[j]->get_lexeme(_ACTION);
                if(ti == tj){
                    // reset solver and expression e
                    e = c.bool_val(true);
                    s.reset();
                    AST* gi = inputTrans[i]->get_first(_PRECONDITION);
                    AST* gj = inputTrans[j]->get_first(_PRECONDITION);
                    if(gi) e = e && ast2expr(gi,module,c,pc);
                    if(gj) e = e && ast2expr(gj,module,c,pc);;
                    z3::expr ei = post2expr(inputTrans[i],module,pc,c);
                    z3::expr ej = post2expr(inputTrans[j],module,pc,c);
                    e = e && (ei != ej);
                    s.add(e);

                    string posi = inputTrans[i]->get_pos();
                    string posj = inputTrans[j]->get_pos();
                    if (s.check()){
                        error_list.append("[WARNING] Non determinism "
                            "may be present due to input transitions "
                            "labeled '"+ ti + "', at " + posi
                            + " and "+ posj + ". Check"
                            " condition 7 for IOSA.\n");
                    }
                    // also check for clocks that are being reset
                    if(!same_rclocks(inputTrans[i], inputTrans[j])){
                        error_list.append("[WARNING] Non determinism "
                            "may be present due to input transitions "
                            "labeled '"+ ti + "', at " + posi
                            + " and "+ posj + " since they don't reset "
                            + "the same clocks. Check condition 7 for IOSA.\n");

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


////////////////////////////////////////////////////////////////////////////////


/* @fill_maps: fill the context @mPc for @ast. Check for variables declarations
               to be correct.
*/
int
Verifier::fill_maps(AST *ast){

    /*NOTE: names of variables and clock can be accessed outside of its
            modules by using <module name>.<variable/clock name>. In the
            type maps this names will be kept under the special module name 
            '#property'. FIXME: we should not be able to access clock names
            for comparisons or reading of their values.
    */

    string error_list = "";

    vector<AST*> modules = ast->get_all_ast(_MODULE);
    for(int i = 0; i < modules.size(); i++){
        vector<AST*> variables = modules[i]->get_all_ast(_VARIABLE);
        vector<AST*> clocks = modules[i]->get_all_ast(_CLOCK);
        string module = modules[i]->get_lexeme(_NAME);
        // Fill the variable type maps
        for(int j=0; j < variables.size(); j++){
            string name = variables[j]->get_lexeme(_NAME);
            string type = variables[j]->get_lexeme(_TYPE);
            if(type == "int"){
                AST* range = variables[j]->get_first(_RANGE);
                if(range){
                    vector<string> limits = range->get_list_lexemes(_NUM);
                    assert(limits.size() == 2);
                    if(stoi(limits[0]) > stoi(limits[1])){
                        string pos = variables[j]->get_pos();
                        error_list.append("[ERROR] Empty range in variable "
                            "declaration at " + pos + ".\n");
                    }
                }else{
                    string pos = variables[j]->get_pos();
                    
                    error_list.append("[ERROR] Missing range for integer "
                        "variable declaration at " + pos + " (" 
                        + module + ").\n");
                }
                mPc->add_var(module,name,mARIT);
                mPc->add_var("#property",module+"."+name,mARIT);
            }else if (type == "bool"){
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

    if(error_list != ""){
        throw error_list;
    }

    return 1;
}


////////////////////////////////////////////////////////////////////////////////


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
                    Type t = get_type( init->get_first(_EXPRESSION), module);
                    if( ((type == "bool") && t != mBOOL) || 
                        ((type == "int" || type == "float") && t != mARIT)){
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
                        throw "[ERROR] Wrong type for transitions "
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
            // enabling clocks
            AST* enable = trans[j]->get_first(_ENABLECLOCK);
            if( enable && 
                !mPc->has_clock(module,enable->get_first(_NAME)->p_name())){
                error_list.append( "[ERROR] No clock named " 
                                 + enable->p_name() + " at " 
                                 + enable->p_pos() + ".\n" );
            }
            // reset clocks
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


////////////////////////////////////////////////////////////////////////////////


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


////////////////////////////////////////////////////////////////////////////////

} // namespace parser

