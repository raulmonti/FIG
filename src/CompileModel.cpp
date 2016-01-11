/*
 */

#include "CompileModel.h"
#include "State.h"
#include "Clock.h"
#include "parser.h"
#include "ModelSuite.h"


using namespace fig;
using namespace std;
using namespace parser;


typedef fig::State<fig::STATE_INTERNAL_TYPE>                state;
typedef fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>  varDec;

namespace{

/**
 * brief Solve a formula and return the value. In boolean formulas 1 would be
 *       true and 0 false.
 * note  For the moment @formula is composed of just a _NUM AST.
 */
int
solve(AST* formula){

    //FIXME for now initialization is just a number.
    int result = 0;
    result = atoi(formula->get_lexeme(_NUM).c_str());
    return result;
}



/**
 * TODO
 **/
const state
CompileVars(const vector<AST*> varList)
{
    vector<varDec> result;
    for(auto const &it: varList){
        string name = it->get_lexeme(_NAME);
        assert(name != "");
        vector<string> limits = {{"0","0"}};
        int init = 0;
        AST* ASTtype = it->get_first(_TYPE);
        AST* ASTrange = it->get_first(_RANGE);
        AST* ASTinit = it->get_first(_INIT);
        if(ASTrange){
            limits = ASTrange->get_list_lexemes(_NUM);
            assert(limits.size() == 2);
        }
        if(ASTinit){
            init = solve(ASTinit);
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
    string pre = ASTpre->toString();
    auto nl = ASTpre->get_all_lexemes(_NAME);
    vector<AST*> assigs = trans->get_all_ast(_ASSIG);
    string lassig; // left side of assignment
    vector<string> rassig; // right side of assignment
    vector<string> variables;
    for(const auto &it: assigs){
        string name = it->get_lexeme(_NAME);
        name.pop_back();
        AST* ASTassig = it->get_first(_EXPRESSION); 
        string assig = ASTassig->toString();
        lassig += assig;
        lassig += ",";
        rassig.push_back(assig);
        vector<string> vars = ASTassig->get_all_lexemes(_NAME);
        variables.insert(variables.end(), vars.begin(), vars.end());
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
    return result;
}

} //namespace


namespace fig{

/**
 * @brief Compile the model in the AST to a FIG simulation model.
 * @param [in] astModel A valid IOSA model.
 * @rise ...
 */
void
CompileModel(AST* astModel){
   
	auto model = fig::ModelSuite::get_instance();
	assert(!model.sealed());

    vector<AST*> modules = astModel->get_all_ast(_MODULE);
    for(auto const &it: modules){
        auto module = CompileModule(it);
        model.add_module(module);
    }
    //TODO SEAL
}

} // namespace fig


