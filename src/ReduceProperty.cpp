#include "ReduceProperty.h"
#include "Ast.h"
#include "FigException.h"
#include <string>
#include <vector>
#include <set>
#include <assert.h>
#include <list>
#include <memory>
#include "Parser.h"
#include "Property.h"
#include "PropertyTransient.h"
#include "Iosacompliance.h"

using namespace std;
using namespace fig;
using namespace parser;


namespace{

string
reduceFormulaRec(const AST* prop, const set<string> &vars)
{
    string result = "true";

    size_t size = prop->branches.size();

    if(size == 0){
        // case of variables and values.
        if(prop->tkn == _NAME){
            if(vars.count(prop->lxm) > 0){
                result = prop->lxm;
            }
        }else if(prop->tkn == _NUM || prop->tkn == _BOOLEAN){
            result = prop->lxm;
        }
    }else if(size == 1){
        // jump level
        AST* b0 = prop->branches[0];
        result = reduceFormulaRec(b0,vars);
    }else if(size == 2){
        // case of negation -- arithmetic (-) or logic (!) --.
        AST* b0 = prop->branches[0];
        AST* b1 = prop->branches[1];
        assert(b0->tkn == _NEGATION || b0->tkn == _MINUS);
        string partialResult = reduceFormulaRec(b1,vars);
        if(partialResult != "true"){
            result = "! " + partialResult;
        }
    }else if(size >= 3){
        AST* b0 = prop->branches[0];
        AST* b1 = prop->branches[1];
        AST* b2 = prop->branches[2];
        // case of subformula -- ( expression ) --
        if(b0->lxm == "("){
            assert(b2->lxm == ")");
            string partialResult = reduceFormulaRec(b1,vars);
            if(partialResult != "true"){
                result = "(" + partialResult + ")";
            }
        }else{
            // case of binary operation
            int level = prop->tkn;
            string r0 = reduceFormulaRec(b0,vars);
            if(level == _EXPRESSION || r0 != "true"){
                result = r0;
                for(int i = 1; i < size-1; i=i+2){
                    b1 = prop->branches[i];
                    b2 = prop->branches[i+1];
                    string r2 =  reduceFormulaRec(b2,vars);
                    if(r2 == "true" && level != _EXPRESSION){
                        return "true";
                    }
                    result += b1->lxm + r2;
                }
            }
        }    
    }
    return result;
}

std::shared_ptr<Property>
reduceTransient(AST* prop,  const set<string> &vars)
{
    assert(prop);
    vector<AST*> formulas = prop->get_all_ast(_EXPRESSION);
    assert(formulas.size() == 2);
    string r0 = reduceFormulaRec(formulas[0], vars);
    string r1 = reduceFormulaRec(formulas[1], vars);

    vector<string> vars0 = formulas[0]->get_all_lexemes(_NAME);
    vector<string> vars1 = formulas[1]->get_all_lexemes(_NAME);
    list<string> reducedVars0;
    list<string> reducedVars1;
    for(const auto &it: vars0){
        if(vars.count(it)>0) reducedVars0.push_back(it);
    }
    for(const auto &it: vars1){
        if(vars.count(it)>0) reducedVars1.push_back(it);
    }
    return std::make_shared< fig::PropertyTransient >(
			r0.c_str(), reducedVars0,  // keep-running condition
			r1.c_str(), reducedVars1);  // goal
}

/*
string
reduceSProp(AST* prop,  const set<string> &vars)
{
    assert(prop);

    string result = "";
    vector<AST*> formulas = prop->get_all_ast(_EXPRESSION);
    assert(formulas.size() == 2);
    string r0 = reduceFormulaRec(formulas[0], vars);
    string r1 = reduceFormulaRec(formulas[1], vars);
    result = "S(" + r0 + " \\ " + r1 + ")";

    return result;
}*/

} //namespace



namespace fig{

std::shared_ptr<Property>
reduceProperty(unsigned int idx, const string &module)
{

    assert(GLOBAL_MODEL_AST);
    assert(GLOBAL_PROP_AST);

    vector<AST*> props = GLOBAL_PROP_AST->get_all_ast(_PROPERTY);
    if(idx >= props.size()){
        throw_FigException("Index out of range.\n");
    }
    AST* prop = props[idx];
    AST* mdl = NULL;
    vector<AST*> modules = GLOBAL_MODEL_AST->get_all_ast(_MODULE);
    for(const auto &it: modules){
        if(it->get_lexeme(_NAME) == module){
            mdl = it;
        }
    }
    if(mdl == NULL){
        throw_FigException("No module named " + module + ".\n");
    }
    vector<AST*> varASTs = mdl->get_all_ast(_VARIABLE);
    set<string> vars;
    for(const auto &it: varASTs){
        vars.insert(it->get_lexeme(_NAME));
    }
    // what kind of property is it?
    AST* pprop = prop->get_first(_PPROP);
    AST* sprop = prop->get_first(_SPROP);
    if(pprop){
        vector<AST*> formulas = pprop->get_all_ast(_EXPRESSION);
        assert(formulas.size() == 2);
        Type t1 = get_type(formulas[0],GLOBAL_PARSING_CONTEXT);
        Type t2 = get_type(formulas[0],GLOBAL_PARSING_CONTEXT);
        if(t1 == T_BOOL && t2 == T_BOOL && NULL == pprop->get_first(_RANGE)){
            return reduceTransient(pprop,vars);
        }
    }
    throw_FigException("Unsupported property \"" + prop->toString() + "\"\n");
}

}

