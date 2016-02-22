//==============================================================================
//
//  IOSA compliance module for FIG
//  Raul Monti
//  2015
//
//==============================================================================

#include <set>
#include <assert.h>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <set>
#include <algorithm>
#include "Parser.h"
#include "Iosacompliance.h"
#include "Ast.h"
#include "Exceptions.h"
#include "FigException.h"


/*TODO decide if the following definition is useful and do the same with
       the rest of long error messages if so.
*/

#define W_0(l1,l2) "[WARNING] Nondeterminism may be present if we reach states"\
                   " where transitions at lines " +  l1 + " and " + l2 + \
                   " are enabled, since they use the same clock and they" \
                   " may reach different states. Check IOSA condition 3.\n"


using namespace std;
using namespace fig;


namespace parser{


/**
 * TODO
 */
bool
is_const_exp(AST* ex)
{
    assert(ex->tkn == _EXPRESSION);
    return ex->get_all_ast(_NAME).empty();
}


/** @brief  Check if the transition parsed into an AST corresponds to an
 *          output transition.
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


/**
 *  @brief Check if @trans1 enabling clock is not reseted by @trans2.
 */
bool
clock_nreset(AST* trans1, AST* trans2){

    string enableCName = "";
    try{
        enableCName = trans1->get_first(_ENABLECLOCK)->get_lexeme(_NAME);
    }catch(std::exception & e){
        throw string("Couldn't find clock\n") + e.what();
    }

    vector<AST*> resetCList = trans2->get_all_ast(_SETC);
	for(size_t i = 0; i < resetCList.size(); i++){
        if(enableCName+"'" == resetCList[i]->get_lexeme(_NAME)) return false;
    }

    return true;
}


/** 
 *  @brief Find out if both transitions have the same enabling clock, if they
 *         have any.
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


/**
 *  @brief Check if two transitions reset the same set of clocks.
 */
bool
same_rclocks(AST* t1, AST* t2){
    vector<string> rc1;
    vector<string> rc2;
    for(auto const &it: t1->get_all_ast(_SETC)){
        rc1.push_back(it->get_lexeme(_NAME));
    }
    for(auto const &it: t2->get_all_ast(_SETC)){
        rc2.push_back(it->get_lexeme(_NAME));
    }
    std::sort(rc1.begin(), rc1.end());
    std::sort(rc2.begin(), rc2.end());
    return  rc1 == rc2;
}



/**
 *  @brief Check if transitions produce the same action.
 */
bool
same_action(AST* t1, AST* t2){

    return t1->get_lexeme(_ACTION) == t2->get_lexeme(_ACTION);

}


/**
 * @brief  interpret a postcondition formula and build the corresponding z3
 *         expression.
 * @return z3 expresion representing the postcondition.
 */
z3::expr
post2expr(AST* pAst, parsingContext &pc, z3::context &c){

    z3::expr pExp = c.bool_val(true);
    set<string> vNameSet;

    /* add the valuations given by the postcondition */
    vector<AST*> pList = pAst->get_all_ast(_ASSIG);
	for(size_t i = 0; i < pList.size(); ++i){
        AST* var = new AST(pList[i]->get_first(_NAME));
        AST* val = pList[i]->get_first(_EXPRESSION);
        vNameSet.insert(pList[i]->get_lexeme(_NAME));
        pExp = pExp && (ast2expr(var,c,pc) == ast2expr(val,c,pc));
    }
    /* add the remaining conditions (unchanged ones) */
    for(auto const &it : pc){
        string vname = it.first;
        Type t = it.second.first;

        if( t != T_CLOCK && t != T_NOTYPE &&
            vname[vname.size()-1] == '\'' && 
            vNameSet.count(vname) <= 0){
            if(t == T_ARIT){
                z3::expr vexpr = c.real_const(vname.c_str());
                pExp = pExp && (vexpr == 
                       c.real_const(vname.substr(0,vname.size()-1).c_str()));
            }else if(t == T_BOOL){
                z3::expr vexpr = c.bool_const(vname.c_str());
                pExp = pExp && (vexpr == 
                       c.bool_const(vname.substr(0,vname.size()-1).c_str()));
            }else{
                assert(false);
            }
        }
    }

    return pExp;
}


//==============================================================================

pair<AST*,AST*>
get_limits(AST* ast, string var){

    pair<AST*,AST*> result;

    vector<AST*> vars = ast->get_all_ast(_VARIABLE);
    for(auto const &it: vars){
        if(it->get_lexeme(_NAME) == var || it->get_lexeme(_NAME)+'\'' == var){
            vector<AST*> limits = 
                it->get_first(_RANGE)->get_all_ast(_EXPRESSION);
            assert(limits.size() == 2);
            result.first = limits[0];
            result.second = limits[1];
            return result;
        }
    }
    throw_FigException("Can't find variable " + var + " limits.");
}

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
ast2expr( AST* formula, context & c, const parsingContext & pc){

    //TODO: assert("check that formula is boolean.");
    expr result = c.bool_val(true);

    int bsize = formula->branches.size();
    if( bsize >= 3){
        AST *b0 = formula->branches[0];
        if( b0->tkn == _SEPARATOR ){
            assert(formula->branches.size() == 3);
            AST *b1 = formula->branches[1];
            AST *b2 = formula->branches[2];
            assert(b2->tkn == _SEPARATOR);
            result = ast2expr(b1,c,pc);
        }else{
            result = ast2expr(b0,c,pc);
            for(size_t i = 1; i < formula->branches.size()-1 ; i=i+2){
                AST *b1 = formula->branches[i];
                AST *b2 = formula->branches[i+1];
                expr e2 = ast2expr(b2,c,pc);
                if(b1->lxm == "+")  result = result + e2;
                else if (b1->lxm == "-") result = result - e2;
                else if (b1->lxm == "*") result = result * e2;
                else if (b1->lxm == "/") result = result / e2;
                else if (b1->lxm == "||") result = result || e2;
                else if (b1->lxm == "&") result = result && e2;
                else if (b1->lxm == ">") result = result > e2;
                else if (b1->lxm == "<") result = result < e2;
                else if (b1->lxm == ">=") result = result >= e2;
                else if (b1->lxm == "<=") result = result <= e2;
                else if (b1->lxm == "==") result = result == e2;
                // FIXME Assignments do not correspond to boolean formulas
                // but this next line helps a lot anyway.
                else if (b1->lxm == "=") result = result == e2;
                else if (b1->lxm == "!=") result = result != e2;
                else {
                    cout << b1->lxm << endl;
                    assert("Wrong symbol!!!\n" && false);
                }
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
                    int t = pc.at(formula->lxm).first;
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
        cout << *formula << endl;
        assert("Wrong number of branches :S\n" && false);
    }

    return result;

}


//==============================================================================

/** @brief Change every variable <name> in an AST to <name'>
 *  @ast   The AST member to be modified.
 */
void
variable_duplicate(AST* ast){

    if(ast->tkn == _NAME){
        ast->lxm = ast->lxm+"'";
    }else{
		for(size_t i = 0; i < ast->branches.size(); ++i){
            variable_duplicate( ast->branches[i]);
        }
    }
}


//==============================================================================
// VERIFIER CLASS IMPLEMENTATION ===============================================
//==============================================================================

/**
 *
 */
Verifier::Verifier(){
}

/**
 *
 */
Verifier::~Verifier(void){}


//==============================================================================

/**
 * @brief   Fully verify if @ast complies with IOSA modeling. Conditions
 *          (5) and (6) are not ensured here, but should be ensured by the
 *          simulation engine, i.e. it should provide initial values for
 *          clocks in every initial state outgoing transitions, and it should
 *          provide self loops for input actions that are no explicitly
 *          declared for every state. Condition (4) cant be checked without
 *          reachability checking. Thus we are only giving out warnings to the
 *          user and we expect this to be correctly checked by the simulation
 *          engine and the behavior should be clear to who ever models with our
 *          simulator.
 *  @return 1 if it complies.
 *  @return 0 otherwise.
 */
void 
Verifier::verify( AST* ast, const parsingContext pc){

    mPc = pc; /* FIXME shouldn't do this copy, it comes from refactoring.
               *       think of a way arround
               */
    // general IOSA properties
    pout << ">> Check names uniqueness...\n";
    names_uniqueness(ast);
    pout << ">> Check typing...\n";
    type_check(ast);
    pout << ">> Check constants...\n";
    //check_constants(ast);
    // IOSA determinism properties
    pout << ">> Check 1st and 2nd IOSA conditions...\n";
    input_output_clocks(ast);
    pout << ">> Check 3rd IOSA condition...\n";
    unique_outputs(ast);
    pout << ">> Check 4th IOSA condition...\n";
    check_exhausted_clocks(ast);
    pout << ">> Check 7th IOSA condition...\n";
    check_input_determinism(ast);
}



//==============================================================================


/**
 */
z3::expr
Verifier::limits2expr(AST* ast, z3::context &c){
	z3::expr result = c.bool_const("true");
    for(auto const &it: mPc){
        string var = it.first;
        Type t = it.second.first;
        if(t == T_ARIT){
            pair<AST*,AST*> limits = get_limits(ast,var);
            result = result && 
                (ast2expr(limits.first,c,mPc) <= c.real_const(var.c_str()) &&
                 c.real_const(var.c_str()) <= ast2expr(limits.second,c,mPc));
        }
    }
    return result;

}


//==============================================================================

/**
 *
 */
bool
Verifier::is_clock(AST* c){

    auto const it = mPc.find(c->get_first(_NAME)->p_name());
    return !(it == mPc.end() || it->second.first != T_CLOCK);

}

/**
 * FIXME reorder methods declaration in the whole module
 */
bool
is_var(AST* c, const parsingContext &pc)
{
    auto const it = pc.find(c->get_first(_NAME)->p_name());
    return ( it != pc.end() && 
             (it->second.first == T_ARIT || it->second.first == T_BOOL));

}

//==============================================================================


/**
 * @brief  check that names that should be unique really are.
 * @return 1 if no duplicated name was found.
 * @throw  string if some duplicated name was found.
 */
int
Verifier::names_uniqueness(AST* ast){

    string error_list = "";
    set<AST*,bool(*)(AST*,AST*)> names 
        ([]( AST* a, AST* b){return a->lxm < b->lxm;});
    vector<AST*> modules = ast->get_all_ast(_MODULE);
    vector<AST*> constants = ast->get_all_ast(_CONST);
    vector<AST*> variables = ast->get_all_ast(_VARIABLE);
    vector<AST*> clocks = ast->get_all_ast(_CLOCK);

    vector<AST*> nameList;    
	for(size_t i = 0; i < modules.size(); ++i){
        nameList.push_back(modules[i]->get_first(_NAME));
    }
	for(size_t i = 0; i < constants.size(); ++i){
        nameList.push_back(constants[i]->get_first(_NAME));
    }
	for(size_t i = 0; i < variables.size(); ++i){
        nameList.push_back(variables[i]->get_first(_NAME));
    }
	for(size_t i = 0; i < clocks.size(); ++i){
        nameList.push_back(clocks[i]->get_first(_NAME));
    }

	for(size_t i = 0; i < nameList.size(); ++i){
        if(!names.insert(nameList[i]).second){
            AST* duplicated = *names.find(nameList[i]);
            error_list.append(("[ERROR] Duplicated name '")
                + nameList[i]->p_name() + "', at " + nameList[i]->p_pos() 
                + ". Previously found at " 
                + duplicated->p_pos() + ".\n");
        }
    }

    // Exception id found duplicated names:
    if(error_list != ""){
        throw_FigException(error_list);
    }
    return 1;
}


//==============================================================================

/**
 *  @brief  Check that input transitions have no clock to wait
 *          for. This is in compliance to IOSA first condition.
 *          Also check that output transitions have exactly one
 *          clock to wait for, in compliance to IOSA second
 *          condition.
 *  @return
 *  @throw
 */
int
Verifier::input_output_clocks(AST* ast){
    // FIXME this will be easier if we force the users to specify ? or !
    // or if we always parse an IO AST even when there is no ? or !
    string error_list = "";
    vector<AST*> modules = ast->get_list(_MODULE);
	for(size_t i = 0; i < modules.size(); i++){
        vector<AST*> transitions = modules[i]->get_list(_TRANSITION);
		for(size_t j = 0; j < transitions.size(); j++){
			AST* ioAst = transitions[j]->get_first(_IO);
            if( ioAst->p_name() == "?" && 
				transitions[j]->get_first(_ENABLECLOCK)){
                error_list.append("[ERROR] In transition declaration at '"
					+ transitions[j]->get_pos() + "'. Input transitions "
                    "should not have to wait for any clocks.\n");
            }
            if( ioAst->p_name() == "!" &&
				!transitions[j]->get_first(_ENABLECLOCK)){
                error_list.append("[ERROR] In transition declaration at '"
					+ transitions[j]->get_pos() +
                    "'. Output transitions should wait for exactly one "
                    "clock.\n");
            }
        }
    }

    if(error_list != ""){
        throw_FigException(error_list); 
    }
    return 1;
}


//==============================================================================

/**
 * @brief: check IOSA condition 3. (without reachability check)
 * @throw:
 * @result:
 * @algorithm: if(  (c1 == c2) 
                 && sat(pre1 && pre2)
                 && (a1 != a2 || C'1 != C'2 || s1 != s2) ) : WARNING
 */

void
Verifier::unique_outputs(AST *ast){
    z3::context c;
    z3::solver s(c);

    vector<AST*> modules = ast->get_list(_MODULE);
	for(size_t i = 0; i < modules.size(); ++i){
        vector<AST*> transs = modules[i]->get_all_ast(_TRANSITION);
		for(size_t i = 0; i < transs.size(); ++i){
			for(size_t j = i+1; j < transs.size(); ++j){
                if( same_clock(transs[i], transs[j])){
                    AST* pre1 = transs[i]->get_first(_PRECONDITION);
                    AST* pre2 = transs[j]->get_first(_PRECONDITION);
                    if(pre1) pre1 = pre1->get_first(_EXPRESSION);
                    if(pre2) pre2 = pre2->get_first(_EXPRESSION);
                    z3::expr e1 = ast2expr(pre1, c, mPc);                            
                    z3::expr e2 = ast2expr(pre2, c, mPc);
                    s.reset();
                    s.add(e1 && e2 && limits2expr(ast,c));
                    if( s.check() ){
                        string line1 = transs[i]->get_line();
                        string line2 = transs[j]->get_line();
                        if(!same_action(transs[i], transs[j])){
                            wout << "[WARNING] Nondeterminism"
                                " may be present if we reach states"
                                " where transitions at lines " +  line1
                                + " and " + line2 + " are enabled, "
                                "since they use the same clock and they"
                                " produce different actions. Check "
                                " IOSA condition 3.\n";
                        }
                        if(!same_rclocks(transs[i], transs[j])){
                            wout << "[WARNING] Nondeterminism"
                                " may be present if we reach states"
                                " where transitions at lines " +  line1
                                + " and " + line2 + " are enabled, "
                                "since they use the same clock and they"
                                " reset different clocks. Check "
                                " IOSA condition 3.\n";
                        }
                        z3::expr p1 = post2expr(transs[i],mPc,c);
                        z3::expr p2 = post2expr(transs[j],mPc,c);
                        s.reset();
                        s.add(e1 && e2 && (p1 != p2) && limits2expr(ast,c));
                        if(s.check()){
                            wout << W_0(line1,line2);
                        }
                    }
                }
            }
        }
    }
}



//==============================================================================

/**
 * @brief Check compliance to condition 4 from IOSA.
 * @Note  Can only partially check due to reachability issues, thus we 
 *        report WARNINGS but we don't ensure presence of non-determinism.
 */       

/*
                {C}   {x}
        []---t2--->[]---t1--->[]
      {x}\
         \
         t3
         \
         v
         []

    TEST:
    x not in C && sat(pre_1(x') && pos_2(x,x') && pre(x) && (!pre_3(x)) ...) 

*/

void
Verifier::check_exhausted_clocks(AST *ast){

    z3::context     c;
    z3::solver      s(c);
    z3::expr        ex          = c.bool_val(true);

    vector<AST*> mods = ast->get_list(_MODULE);
	for (size_t i = 0 ; i < mods.size() ; i++){
        vector<AST*> transs = mods[i]->get_all_ast(_TRANSITION);
		for(size_t i = 0; i < transs.size(); i++){
            if(is_output(transs[i])){
				for(size_t j = 0; j < transs.size(); ++j){
                    s.reset();
                    /*Check that i and j are different transitions and that 
                      j does not reset the enabling clock from i.*/
                    if(i != j && clock_nreset(transs[i],transs[j])){
                        AST *pre = transs[i]->get_first(_PRECONDITION);
                        if(pre){
                            AST *g1 = new AST(pre);
                            /*g1 is evaluated after the transition.*/
                            variable_duplicate(g1);
                            ex = ast2expr(g1,c,mPc);
                            delete g1;
                        }
                        AST *p2 = transs[j]->get_first(_POSTCONDITION);
                        if(p2){
                            ex = ex && (post2expr(p2,mPc,c));
                        }
                        AST * g2 = transs[j]->get_first(_PRECONDITION);
                        if(g2){
                            ex = ex && ast2expr(g2,c,mPc);
                        }
						for(size_t k = 0; k < transs.size(); ++k){
                            /* !g for guards of every output transitions 
                               going out from the same state as j
                               and same clock as i.
                            */
                            if(j != k && same_clock(transs[i], transs[k])){
                                AST *g = transs[k]->get_first(_PRECONDITION);
                                if(g){
                                    ex = ex && (! ast2expr(g,c,mPc));
                                }
                            }
                        }
                        s.add(ex && limits2expr(ast,c));
                        if(s.check()){
                            string namei = transs[i]->get_lexeme(_ACTION);
                            string namej = transs[j]->get_lexeme(_ACTION);
                            string linei = transs[i]->get_line();
                            string linej = transs[j]->get_line();
                            wout << "[WARNING] it is possible that"
                                   " transition " + namei
                                 + " at line " + linei + " could be reached "
                                 + "with its clock exhausted via transition " 
                                 + namej + " at line " + linej + ", possibly " 
                                 + "causing non determinisim. Check IOSA "
                                 + "condition 4.\n";
                        }
                    }
                }
            }
        }
    }
}



//==============================================================================

/**
 *  @brief Check condition 7 for IOSA.
 *  @algorithm 
 *      if (act1 = act2):
 *          if (sat ( pre1(x) && pre2(x)  //There is a valuation for both pre 
 *              && (pos1(x,x') != pos2(x,x'))) //that makes both post different
 *              ):
 *              WARNING.
 */

void
Verifier::check_input_determinism(AST *ast){

    string error_list = "";
    vector<AST*> modules = ast->get_list(_MODULE);
    z3::context c;
    z3::expr e = c.bool_val(true);
    z3::solver s(c);

	for(size_t m = 0; m < modules.size(); ++m){
        vector<AST*> inputTrans;
        vector<AST*> trans = modules[m]->get_all_ast(_TRANSITION);
		for(size_t t = 0; t < trans.size(); ++t){
            if(!is_output(trans[t])){
                inputTrans.push_back(trans[t]);
            }
        }
		for(size_t i = 0; i < inputTrans.size(); ++i){
			for(size_t j = i+1; j < inputTrans.size(); ++j){
                string ti = inputTrans[i]->get_lexeme(_ACTION);
                string tj = inputTrans[j]->get_lexeme(_ACTION);
                if(ti == tj){
                    // reset solver and expression e
                    e = c.bool_val(true);
                    s.reset();
                    AST* gi = inputTrans[i]->get_first(_PRECONDITION);
                    AST* gj = inputTrans[j]->get_first(_PRECONDITION);
                    if(gi) e = e && ast2expr(gi,c,mPc);
                    if(gj) e = e && ast2expr(gj,c,mPc);;
                    z3::expr ei = post2expr(inputTrans[i],mPc,c);
                    z3::expr ej = post2expr(inputTrans[j],mPc,c);
                    e = e && (ei != ej);
                    s.add(e && limits2expr(ast,c));
                    string posi = inputTrans[i]->get_pos();
                    string posj = inputTrans[j]->get_pos();
                    if (s.check()){
                        wout << "[WARNING] Non determinism "
                            "may be present due to input transitions "
                            "labeled '"+ ti + "', at " + posi
                            + " and "+ posj + ". Check"
                            " condition 7 for IOSA.\n";
                    }
                    // also check for clocks that are being reset
                    if(!same_rclocks(inputTrans[i], inputTrans[j])){
                        wout << "[WARNING] Non determinism "
                            "may be present due to input transitions "
                            "labeled '"+ ti + "', at " + posi
                            + " and "+ posj + " since they don't reset "
                            + "the same clocks. Check condition 7 for IOSA.\n";

                    }                    
                }                
            }
        }
    }
}



//==============================================================================

/**
 *  @brief Type check expressions in the model.
 *  @throw String error.
 *  @param [in] ast Should be an AST instance of a parsed model.
 */

int
Verifier::type_check(AST *ast){

    string error_list = "";
    int result = 1;

    vector<AST*> variables = ast->get_all_ast(_VARIABLE);

    // Check variable initialization; and range (in case of aritmetic type).
	for(size_t i=0;i<variables.size();i++){
        string vname = variables[i]->get_lexeme(_NAME);
        Type v_t = mPc[vname].first;
        Type i_t = T_NOTYPE;
        AST* ASTinit = variables[i]->get_first(_INIT);
        string init = "";
        AST* iexp = NULL;
        // check initialization
        if(ASTinit){
            iexp = ASTinit->get_first(_EXPRESSION);
            assert(iexp);
            i_t = get_type(iexp,mPc);
            init = solve_const_expr(iexp,mPc);
            if(i_t != v_t){
                error_list.append("[ERROR] Wrong type for variable "
                                  "initialization at "
                                  + variables[i]->get_pos() + ".\n");
            }
        }
        // check range in case of aritmetic type.
        if(v_t == T_ARIT){
            AST* range = variables[i]->get_first(_RANGE);
            vector<AST*> limits = range->get_list(_EXPRESSION);
            assert(limits.size()==2);
            if(T_ARIT != get_type(limits[0],mPc)){
                error_list.append("[ERROR] Wrong type for integer range at "
                                  + limits[0]->get_pos() + ".\n");
            }
            if(T_ARIT != get_type(limits[1],mPc)){
                error_list.append("[ERROR] Wrong type for integer range at "
                                  + limits[1]->get_pos() + ".\n");

            }
            int ll = stoi(solve_const_expr(limits[0],mPc));
            int ul = stoi(solve_const_expr(limits[1],mPc));
            if( ll > ul){
                error_list.append("[ERROR] Empty range for variable definition "
                                  "at " + variables[i]->get_pos() + ".\n");
            }
            if(ASTinit && i_t == T_ARIT){
                int in = stoi(init);
                if(ll>in || in>ul){
                    error_list.append("[ERROR] Initialization out of range for "
                                      "variable definition at "
                                      + iexp->get_pos() + ".\n");
                }
            }
        }else if(v_t != T_BOOL){
            assert(false && "WRONG TYPE FOR VARIABLE");
        }
 
    }

    // Type check transitions preconditions:
    vector<AST*> trans = ast->get_all_ast(_TRANSITION);
	for(size_t i =0; i < trans.size(); ++i){
        AST *pre = trans[i]->get_first(_PRECONDITION);
        if (pre){
            AST *expr = pre->get_first(_EXPRESSION);
            if( T_BOOL != get_type(expr,mPc)){
                error_list.append( "[ERROR] Wrong type for transitions "
                    "precondition at " + expr->p_pos() 
                    + ". It should be boolean but found "
                    "arithmetic instead.\n");
            }
        }
    }

    // Type check assignments in post conditions
	for(size_t i = 0; i < trans.size(); ++i){
        vector<AST*> assigs = trans[i]->get_all_ast(_ASSIG);
		for(size_t j = 0; j < assigs.size(); ++j){
            AST* var = assigs[j]->get_first(_NAME);
            string vname = assigs[j]->get_lexeme(_NAME);
            AST* expr = assigs[j]->get_first(_EXPRESSION);
            try{
                Type t_v = mPc[vname].first;
                Type t_e = get_type(expr,mPc);
                if(t_v != t_e){
                    throw "[ERROR] Wrong type in assignment of variable "
                        + vname + " at " + var->p_pos() + ".\n";
                }
            }catch(string err){
                error_list.append(err);
            }catch(const out_of_range err){
                error_list.append( "[ERROR] Undeclared variable " 
                     + vname + " at " + var->p_pos() + ".\n");
            }
        }
    }

    /* Check that enabling clock and reseting clocks are really clocks */
    map<string, AST*> rclocks;
	for(size_t j = 0; j < trans.size(); j++){
        // enabling clocks
        AST* enable = trans[j]->get_first(_ENABLECLOCK);
        if(enable){
            auto const it = mPc.find(enable->get_first(_NAME)->p_name());
            if( enable && (it == mPc.end() || it->second.first != T_CLOCK )){
                error_list.append( "[ERROR] No clock named " 
                    + enable->p_name() + " at " + enable->p_pos() + ".\n" );
            }
        }
        // reset clocks
        vector<AST*> resets = trans[j]->get_all_ast(_SETC);
		for(size_t k = 0; k < resets.size(); ++k){
            string name = resets[k]->get_first(_NAME)->p_name();
            name.pop_back();
            AST* distr = resets[k]->get_first(_DISTRIBUTION);
            auto const it = mPc.find(name);
            if(it == mPc.end() || it->second.first != T_CLOCK){
                error_list.append( "[ERROR] No clock named " + name 
                                 + " at " + resets[k]->p_pos() + ".\n" );
            }else{
                AST* exp = distr->get_first(_EXPRESSION);
                if(!is_const_exp(exp)){
                    error_list.append("Non constant expression found as "
                        "distribution parameter at " + exp->get_pos() + ".\n");
                }
                // Check unique distribution for each clock.
                auto ret = rclocks.insert(make_pair(name, distr));
                if(!ret.second){
                    if(*distr != *ret.first->second){
                        string pos1 = distr->get_pos();
                        string pos2 = ret.first->second->get_pos();
                        error_list.append("[ERROR] Different distributions "
                            "have been defined for clock '" + name + "', at " +
                            pos1 + " and " + pos2 + ".\n");
                    }
                }
            }
        }
    }
    // Every enabling clock should have a defined distribution ...
    vector<AST*> eclist = ast->get_all_ast(_ENABLECLOCK);
    vector<AST*> rclist = ast->get_all_ast(_SETC);
    set<string>  rcset;
    for(const auto &it: rclist){
        string name = it->get_lexeme(_NAME);
        name.pop_back();
        rcset.insert(name);
    }
    for(const auto &it: eclist){
        string name = it->get_lexeme(_NAME);
        if(rcset.find(name) == rcset.end()){
            error_list.append("[ERROR] No distribution found for clock "
                + name + " at " + it->get_pos() + "\n");
        }
    }

    if(error_list != ""){
        throw_FigException(error_list);
    }

    return result;
}

//==============================================================================

/**
 *  @brief Return type of an expression.
 *  @return Type of the expression if it has one.
 *  @throw String with error message if there is something wrong with typing.
 */
Type
get_type(AST *expr, const parsingContext &pc)
{
    assert(expr);
    if(expr->tkn == _EXPRESSION){
        AST *equality = expr->get_branch(0);
        Type t1 = get_type(equality,pc);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,pc);
            if(t1 != T_BOOL || t1 != t2){
                throw_FigException("[ERROR] Wrong types for binary operator '" 
                    + op->p_name() + "', at " + op->p_pos() + ".\n");
            }else{
                return T_BOOL; 
            }
        }
        return t1;
    }else if(expr->tkn == _EQUALITY){
        AST *comparison = expr->get_branch(0);
        Type t1 = get_type(comparison,pc);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,pc);
            if(t1 != t2){
                throw_FigException("[ERROR] Wrong types for equality "
                    "operator at " + op->p_pos() + ".\n");
            }else{
                return T_BOOL; 
            }
        }
        return t1;
    }else if(expr->tkn == _COMPARISON){
        AST *summation = expr->get_branch(0);
        Type t1 = get_type(summation,pc);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,pc);
            if(t1 != T_ARIT || t1 != t2){
                throw_FigException("[ERROR] Wrong types for arithmetic "
                    "comparison at " + op->p_pos() + ".\n");
            }else{
                return T_BOOL; 
            }
        }
        return t1;
    }else if(expr->tkn == _SUM){
        AST *division = expr->get_branch(0);
        Type t1 = get_type(division,pc);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,pc);
            if(t1 != T_ARIT || t1 != t2){
                throw_FigException("[ERROR] Wrong types for arithmetic "
                    "operation at " + op->p_pos() + ".\n");
            }else{
                return T_ARIT; 
            }
        }
        return t1;
    }else if(expr->tkn == _DIV){
        AST *value = expr->get_branch(0);
        Type t1 = get_type(value,pc);
        AST *op = expr->get_branch(1);
        AST *expr2 = expr->get_branch(2);
        if(op){
            Type t2 = get_type(expr2,pc);
            // FIXME should check division by zero????
            if(t1 != T_ARIT || t1 != t2){
                throw_FigException("[ERROR] Wrong types for arithmetic "
                    "operation at " + op->p_pos() + ".\n");
            }else{
                return T_ARIT; 
            }
        }
        return t1;
    }else if(expr->tkn == _VALUE){
        AST *value = expr->get_branch(0);
        Type t;
        switch (value->tkn){
            case _NAME:
                if (is_var(value,pc)){
                    t = pc.at(value->lxm).first;
                }else{
                    throw_FigException("[ERROR] Undeclared variable '" 
                        + value->lxm + "' at " + value->p_pos() + ".\n");
                }
                break;
            case _BOOLEAN:
                t = T_BOOL;
                break;
            case _NUM:
                t = T_ARIT;
                break;
            case _SEPARATOR:
                t = get_type(expr->get_branch(1),pc);
                break;
            case _NEGATION:
                t = get_type(expr->get_branch(1),pc);
                if(t != T_BOOL){
                    throw_FigException("[ERROR] Wrong type for boolean"
                        " negation, at " + value->p_pos() + ".\n");
                }
                break;
            case _MINUS:
                t = get_type(expr->get_branch(1),pc);
                if(t != T_ARIT){
                    throw_FigException("[ERROR] Wrong type for arithmetic "
                        "negation, at " + value->p_pos() + ".\n");
                }
                break;
            default:
                assert(false);
                break;
        }
        return t;
    }else{
        cout << expr->tkn << " " << expr->lxm << endl;
        assert(false);
    }
    assert(0);
}


//==============================================================================

string
solve_const_expr(AST* ex, const parsingContext &pc)
{
    if(!is_const_exp(ex)){
        throw FigNotConstant( "Found not constant expression <" + ex->toString() 
                              + ">."
                            , stoi(ex->get_line())
                            , stoi(ex->get_column()));
    }

    string result;
    z3::context c;
    z3::solver s(c);

    z3::expr res(c);
    Type t = get_type(ex,pc);
    if(t==T_ARIT){
        res = c.real_const("#");
    }else{
        assert(t==T_BOOL);
        res = c.bool_const("#");
    }
    z3::expr e = ast2expr(ex,c,pc);
    s.add(res == e);
    
    assert(sat==s.check());
    auto m = s.get_model();

    // FIXME round down to closest int.
    if(t==T_ARIT){
        result = Z3_get_numeral_string(c, m.eval(res,false));
    }else{
        if(Z3_get_bool_value(c,m.eval(res,false))){
            result = "true";
        }else{
            result = "false";
        }
    }
    return result;
}
//==============================================================================

} // namespace parser

