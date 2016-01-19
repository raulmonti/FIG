/*
 */

#include <set>
#include <vector>
#include <map>
#include "CompileModel.h"
#include "State.h"
#include "Clock.h"
#include "parser.h"
#include "ModelSuite.h"
#include "iosacompliance.h" // ast2expr
#include "FigException.h"
#include <z3++.h>


using namespace fig;
using namespace std;
using namespace parser;


typedef fig::State<fig::STATE_INTERNAL_TYPE>                state;
typedef fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>  varDec;



namespace{

/**
 * TODO
 */
map<string,string>
solve_constant_defs(vector<AST*> defs, const parsingContext &pc){
    z3::context c;
    z3::solver s(c);
    vector<string> names;
    vector<expr> exprs;
    map<string,string> result;
    for(const auto &it: defs){
        string n = it->get_lexeme(_NAME);
        names.push_back(n);
        z3::expr v(c);
        if(pc.at(n).first==T_ARIT){
            v = c.real_const(n.c_str());
        }else{
            v = c.bool_const(n.c_str());
        }
        exprs.push_back(v);
        z3::expr e = ast2expr(it->get_first(_EXPRESSION),c,pc);
        s.add(v == e);
    }
    assert(sat==s.check());
    auto m = s.get_model();
    for(int i = 0; i < exprs.size(); ++i){
        if(pc.at(names[i]).first==T_ARIT){
            result[names[i]] = Z3_get_numeral_string(
                                c
                               ,m.eval(exprs[i],false));
        }else{
            if(Z3_get_bool_value(c,m.eval(exprs[i],false))){
                result[names[i]] = "true";
            }else{
                result[names[i]] = "false";
            }
        }
    }
    return result;
}




void
rec_circ_depend(string start, const map<string, vector<string>> &depend){

    static set<string> stack;
    auto ret = stack.insert(start);
    if(!ret.second){
        string err = "";
        for(const auto &it: stack){
            err += it;
            err += " ";
        }
        stack.clear();
        throw FigException(err);
    }
    for(const auto &it: depend.at(start)){
        rec_circ_depend(it, depend);
    }
    stack.erase(start);
}


void
check_no_const_circular_depend(AST* model)
{
    vector<AST*> aux = model->get_all_ast(_CONST);
    map<string, vector<string>> depend;

    for(const auto &it: aux){
        string name = it->get_lexeme(_NAME);
        AST* expr = it->get_first(_EXPRESSION);
        vector<string> depends = expr->get_all_lexemes(_NAME);
        depend[name] = depends;
    }

    for(const auto &it: depend){
        try{
            rec_circ_depend(it.first, depend);
        }catch(FigException &e){
            throw FigException("[ERROR] Circular dependency on definition"
                               " of constant " + it.first + ". Stack: " + 
                               e.msg());
        }
    }
}



/**
 * TODO
 **/
const state
CompileVars(const vector<AST*> varList, const parsingContext &pc)
{
    vector<varDec> result;
    for(auto const &it: varList){
        string name = it->get_lexeme(_NAME);
        assert(name != "");
        vector<string> limits = {{"0","0"}};
        int init = 0;
        Type t = pc.at(name).first;
        AST* ASTrange = it->get_first(_RANGE);
        AST* ASTinit = it->get_first(_INIT);
        if(ASTrange){
            limits = ASTrange->get_list_lexemes(_NUM);
            assert(limits.size() == 2);
        }
        if(ASTinit){
            AST* num = ASTinit->get_first(_NUM);
            AST* boo = ASTinit->get_first(_BOOLEAN);
            AST* con = ASTinit->get_first(_NAME);
            if(num != ""){
                init = atoi(
        }
        result.push_back(make_tuple(name
                                   ,atoi(limits[0].c_str())
                                   ,atoi(limits[1].c_str())
                                   ,init));
    }
    return result;
}


/**
 * brief Get a vector with each clock from the model. 
 **/
const vector<Clock>
CompileClocks(const vector<AST*> transitions)
{
    vector<Clock> result;
    vector<AST*> clocks; 
    for(const auto &it: transitions){
        const vector<AST*> resetCks = it->get_all_ast(_SETC);
        clocks.insert(clocks.end(),resetCks.begin(), resetCks.end());
    }

    for(const auto &it: clocks){
        string name = (it->get_lexeme(_NAME));
        name.pop_back();
        string distrib = it->get_first(_DISTRIBUTION)->get_lexeme(_NAME);
        vector<string> params = 
            it->get_first(_DISTRIBUTION)->get_all_lexemes(_NUM);
        // FIXME que feo que es armar el array ... no se de que otra forma
        //       hacerlo.
        fig::DistributionParameters dParams;
        fig::DistributionParameters::iterator dpit = dParams.begin();
        for(const auto &pit: params){
            *dpit = atoi(pit.c_str());
            dpit++;
        }
        result.push_back(Clock(name, distrib, dParams));
    }
    return result;
}


/**
 * TODO
 **/
fig::Transition
CompileTransition(AST* trans){

    string action = trans->get_lexeme(_ACTION);
    bool io = trans->get_lexeme(_IO) == "!";
    string eclk = "";
    if(io){
        eclk = trans->get_first(_ENABLECLOCK)->get_lexeme(_NAME);
    }
    AST* ASTpre = trans->get_first(_PRECONDITION);
    string pre = "";
    vector<string> nl;
    if(ASTpre){
        pre = ASTpre->toString();
        nl = ASTpre->get_all_lexemes(_NAME);
    }
    vector<AST*> assigs = trans->get_all_ast(_ASSIG);
    string lassig; // left side of assignment
    vector<string> rassig; // right side of assignment
    vector<string> variables;
    for(const auto &it: assigs){
        string name = it->get_lexeme(_NAME);
        name.pop_back();
        AST* ASTassig = it->get_first(_EXPRESSION); 
        if(ASTassig){        
            string assig = ASTassig->toString();
            lassig += assig;
            lassig += ",";
            rassig.push_back(assig);
            vector<string> vars = ASTassig->get_all_lexemes(_NAME);
            variables.insert(variables.end(), vars.begin(), vars.end());
        }
    }
    if(lassig != ""){
        lassig.pop_back();    
    }
    vector<string> setcs;
    for(const auto &it: trans->get_all_ast(_SETC)){
        string sc = it->get_lexeme(_NAME);
        sc.pop_back();
        setcs.push_back(sc);
    }

    fig::Transition result( Label(action,io)
                          , eclk
                          , Precondition(pre, nl)
                          , Postcondition(lassig,rassig,variables)
                          , setcs);

    return result;
}


/**
 * TODO
 */
bool
is_input(AST* transition){
    return("?" == transition->get_lexeme(_IO));
}

/**
 * TODO
 */
vector<fig::Transition>
build_input_enable( vector<AST*> transitions)
{
    vector<fig::Transition> result;
    map<string, vector<AST*>> labelToPre;
    for(const auto &it: transitions){
        if(is_input(it)){
            string l = it->get_lexeme(_ACTION);
            AST* pre = it->get_first(_PRECONDITION);
            auto ret = labelToPre.find(l);
            if(ret == labelToPre.end()){
                labelToPre[l] = vector<AST*>(1,pre);
            }else{
                labelToPre[l].push_back(pre);
            }
        }
    }
    for(const auto &it: labelToPre){
        string pre = "true";
        vector<string> preVars;        
        for(const auto &itpre: it.second){
            if(itpre){
                pre += "& !(";
                pre += itpre->toString();
                pre += ")";
                vector<string> aux = itpre->get_all_lexemes(_NAME);
                preVars.insert(preVars.end(),aux.begin(),aux.end());
            }
        }
        vector <string> dummy;
        fig::Transition t(Label(it.first,false)
                         , ""
                         , Precondition(pre,preVars)
                         , Postcondition("",dummy,dummy)
                         , dummy
                         );
        result.push_back(t);
    }
    
    return result;
}

/**
 * TODO
 **/
std::shared_ptr<ModuleInstance> 
CompileModule(AST* module)
{
    string name = module->get_lexeme(_NAME);
    vector<AST*> variables = module->get_all_ast(_VARIABLE);
    vector<AST*> transitions = module->get_all_ast(_TRANSITION);

    auto result = make_shared<ModuleInstance>(name
                                ,CompileVars(variables)
                                ,CompileClocks(transitions));

    for(const auto &it: transitions){
        auto transition = CompileTransition(it);
        result->add_transition(transition);
    }
    for(const auto &it: build_input_enable(transitions)){
        result->add_transition(it);
    }
    return result;
}

} //namespace







namespace fig{

/**
 * @brief Compile the model in the AST to a FIG simulation model.
 * @param [in] astModel A valid IOSA model.
 * @throw FigException
 */
void
CompileModel(AST* astModel, const parsingContext &pc){
   
	auto model = fig::ModelSuite::get_instance();
	assert(!model.sealed());

    // replace constants
    check_no_const_circular_depend(astModel);
    vector<AST*> consts = astModel->get_all_ast(_CONST);
    solve_constant_defs(consts, pc);
    

    vector<AST*> modules = astModel->get_all_ast(_MODULE);
    for(auto const &it: modules){
        auto module = CompileModule(it);
        model.add_module(module);
    }
    //TODO SEAL
}

} // namespace fig


