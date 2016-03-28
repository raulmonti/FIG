#include <set>
#include <vector>
#include <iterator>   // std::begin(), std::end()
#include <algorithm>  // std::find_if()
#include <map>
#include "CompileModel.h"
#include "State.h"
#include "Clock.h"
#include "Parser.h"
#include "ModelSuite.h"
#include "Iosacompliance.h"
#include "Exceptions.h"
#include "FigException.h"
#include "Property.h"
#include "PropertyTransient.h"
#include "PropertyRate.h"
#include <z3++.h>


// ADL
using std::begin;
using std::end;

/// @todo Raul, I don't recommend this, at least for std
///       If you will declare only the STL used elements, like in "ADL" above
using namespace fig;
using namespace std;
using namespace parser;




namespace{

const state
CompileVars(const vector<AST*> varList, const parsingContext &pc)
{
    vector<varDec> result;
    for(auto const &it: varList){
        string name = it->get_lexeme(_NAME);
        assert(name != "");
        vector<string> limits(2,"0");
		string init;
        AST* ASTrange = it->get_first(_RANGE);
        AST* ASTinit = it->get_first(_INIT);
        if(ASTrange){
            vector<AST*> ASTlimits = ASTrange->get_all_ast(_EXPRESSION);
            assert(ASTlimits.size() == 2);
            limits[0] = solve_const_expr(ASTlimits[0],pc);
            limits[1] = solve_const_expr(ASTlimits[1],pc);
		} else {
			// For now assume boolean;
			// revise whenever we start accepting initialization lists
			limits[0] = "0";
			limits[1] = "1";
		}
        if(ASTinit){
			AST* ASTexp = ASTinit->get_first(_EXPRESSION);
            Type t = get_type(ASTexp,pc);
            string exp = solve_const_expr(ASTexp,pc);
			if(t == T_ARIT) {
				init = exp.c_str();
			} else if(t == T_BOOL) {
				init = exp == "true" ? "1" : "0";
            }
		} else {
			// Default to smallest value when no explicit initialization
			init = limits[0];
		}
        result.push_back(make_tuple(name
                                   ,atoi(limits[0].c_str())
                                   ,atoi(limits[1].c_str())
								   ,atoi(init.c_str())));
	}
    return result;
}

//==============================================================================

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
		// we parse clocks as they're set, and they could be set several times
		if (end(result) != std::find_if(begin(result), end(result),
				[&name](const Clock& clk){ return clk.name() == name; })) {
			continue;  // avoid adding a clock which was already parsed
		}
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

//==============================================================================

fig::Transition
CompileTransition(AST* trans)
{

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
            rassig.push_back(name);
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
                          , Postcondition(lassig,variables,rassig)
                          , setcs);
    return result;
}


//==============================================================================

vector<fig::Transition>
build_input_enable( vector<AST*> transitions)
{
    vector<fig::Transition> result;
    map<string, vector<AST*>> labelToPre;
    for(const auto &it: transitions){
        if("?" == it->get_lexeme(_IO)){ // is it input?
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

//==============================================================================

std::shared_ptr<ModuleInstance> 
CompileModule(AST* module, const parsingContext &pc)
{
    string name = module->get_lexeme(_NAME);
    vector<AST*> variables = module->get_all_ast(_VARIABLE);
    vector<AST*> transitions = module->get_all_ast(_TRANSITION);

    auto result = make_shared<ModuleInstance>(name
                                             ,CompileVars(variables,pc)
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


//==============================================================================

std::shared_ptr<Property> 
CompileTransient(AST* prop)
{
    assert(prop);
    vector<AST*> formulas = prop->get_list(_EXPRESSION);
    assert(formulas.size() == 2);
    string r0 = formulas[0]->toString();
    string r1 = formulas[1]->toString();
    vector<string> vars0 = formulas[0]->get_all_lexemes(_NAME);
    vector<string> vars1 = formulas[1]->get_all_lexemes(_NAME);
    return std::make_shared< fig::PropertyTransient >(
	    r0.c_str(), vars0,  // keep-running condition
		r1.c_str(), vars1);  // goal
}

//==============================================================================

std::shared_ptr<Property> 
CompileSSRate(AST* prop)
{
    assert(prop);
    vector<AST*> formulas = prop->get_list(_EXPRESSION);
    assert(formulas.size() == 1);
    string r0 = formulas[0]->toString();
    vector<string> vars0 = formulas[0]->get_all_lexemes(_NAME);
    return std::make_shared< fig::PropertyRate >(
	    r0.c_str(), vars0);
}



//==============================================================================

std::shared_ptr<Property> 
CompileProperty(AST* prop)
{
    assert(prop);
    AST* pprop = prop->get_first(_PPROP);
    if(pprop){
        vector<AST*> formulas = pprop->get_list(_EXPRESSION);
        assert(formulas.size() == 2);
        Type t1 = get_type(formulas[0],GLOBAL_PARSING_CONTEXT);
        Type t2 = get_type(formulas[1],GLOBAL_PARSING_CONTEXT);
        if(t1 == T_BOOL && t2 == T_BOOL && NULL == pprop->get_first(_RANGE)){
            return CompileTransient(pprop);
        }else{
            wout << "[WARNING] Unsupported transient property " 
                << prop->toString() << endl;
            return NULL;
        }
    }

    AST* sprop = prop->get_first(_SPROP);
    if(sprop){
        vector<AST*> formulas = sprop->get_list(_EXPRESSION);
        assert(formulas.size() >= 1);
        assert(formulas.size() <= 2);
        if(formulas.size()==1){
            return CompileSSRate(sprop);
        }else{
            wout << "[WARNING] Unsupported steady state property " 
                << prop->toString() << endl;
            return NULL;
        }
    }
    
    assert(false); // Strange property ...
}






} //namespace





namespace fig{

/**
 * @brief Compile the model in the AST to a FIG simulation model.
 * @param [in] astModel A valid IOSA model.
 * @throw FigException
 */
void
CompileModel(AST* astModel, const parsingContext &pc)
{   
    assert(astModel);
	auto model = fig::ModelSuite::get_instance();
	assert(!model.sealed());
    vector<AST*> modules = astModel->get_all_ast(_MODULE);
	for(const auto& it: modules){
		std::shared_ptr< ModuleInstance > module(nullptr);
		module = CompileModule(it,pc);
		model.add_module(module);
	}
    for(const auto &it: GLOBAL_PROP_AST->get_all_ast(_PROPERTY)){
        std::shared_ptr< Property > prop(nullptr);
        prop = CompileProperty(it);
        if(prop){
            model.add_property(prop);
        }
    }

	model.seal();
}

} // namespace fig


