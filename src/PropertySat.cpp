#include "PropertySat.h"
#include "Parser.h"
#include "Ast.h"
#include "Iosacompliance.h"
#include <set>
#include <z3_api.h>

using namespace std;
using namespace parser;
using namespace fig;


z3::expr
build_range(AST* ast, z3::context &c){

    assert(ast->tkn == parser::_VARIABLE);
    z3::expr result = c.bool_val(true);
    string vname = ast->get_lexeme(_NAME);
	vector<AST*> astLimits= ast->get_first(_RANGE)->get_all_ast(_EXPRESSION);
    assert(astLimits.size() == 2);
    
    z3::expr e_v = c.int_const(vname.c_str());
    z3::expr e_l = ast2expr(astLimits[0],c,GLOBAL_PARSING_CONTEXT);
    z3::expr e_u = ast2expr(astLimits[1],c,GLOBAL_PARSING_CONTEXT);

    result = e_l <= e_v;
    result = result && e_v <= e_u;

    return result;
}

//==============================================================================


PropertySat::PropertySat(unsigned int idx, vector<string> &vnames):
limitsExpr_(c_)
{
    assert(GLOBAL_PROP_AST);

    vNames_ = vnames;
    vector<AST*> ASTprops = GLOBAL_PROP_AST->get_all_ast(_PROPERTY);
    assert((size_t)idx < ASTprops.size());
    for(const auto &it: ASTprops[idx]->get_all_ast(_EXPRESSION)){
        if(T_BOOL == get_type(it, GLOBAL_PARSING_CONTEXT)){
            propExpr_.push_back(parser::ast2expr(it, c_
                , GLOBAL_PARSING_CONTEXT));
        }
    }
    
    // build variable range constraints
    limitsExpr_ = c_.bool_val(true);
    set<string> vnSet(vnames.begin(),vnames.end());
    for(const auto &it: GLOBAL_MODEL_AST->get_all_ast(_VARIABLE)){
        string name = it->get_lexeme(_NAME);
        if(vnSet.count(name)>0){
            if(GLOBAL_PARSING_CONTEXT[name].first == T_ARIT){
                limitsExpr_ = limitsExpr_ && build_range(it,c_);
            }
        } 
    }
}

//==============================================================================


PropertySat::PropertySat(unsigned int idx, vector<string> &&vnames):
limitsExpr_(c_),
vNames_(vnames)
{
	assert(GLOBAL_PROP_AST);

	vector<AST*> ASTprops = GLOBAL_PROP_AST->get_all_ast(_PROPERTY);
	assert((size_t)idx < ASTprops.size());
	for(const auto &it: ASTprops[idx]->get_all_ast(_EXPRESSION)){
		if(T_BOOL == get_type(it, GLOBAL_PARSING_CONTEXT)){
			propExpr_.push_back(parser::ast2expr(it, c_
				, GLOBAL_PARSING_CONTEXT));
		}
	}

	// build variable range constraints
	limitsExpr_ = c_.bool_val(true);
	set<string> vnSet(vnames.begin(),vnames.end());
	for(const auto &it: GLOBAL_MODEL_AST->get_all_ast(_VARIABLE)){
		string name = it->get_lexeme(_NAME);
		if(vnSet.count(name)>0){
			if(GLOBAL_PARSING_CONTEXT[name].first == T_ARIT){
				limitsExpr_ = limitsExpr_ && build_range(it,c_);
			}
		}
	}
}

//==============================================================================


bool
PropertySat::sat(unsigned int idx, std::vector<STATE_INTERNAL_TYPE> valuation)
{
    return general_sat(idx, false, valuation);
}

//==============================================================================


bool
PropertySat::nsat(unsigned int idx, std::vector<STATE_INTERNAL_TYPE> valuation)
{
    return general_sat(idx, true, valuation);
}

//==============================================================================


bool
PropertySat::general_sat(unsigned int idx,
                         bool negation,
                         std::vector<STATE_INTERNAL_TYPE> valuation)
{
    assert(idx < propExpr_.size());
    z3::solver s(c_);
    z3::expr val_expr = c_.bool_val(true);
    for(size_t i = 0; i < valuation.size(); ++i){
        if(GLOBAL_PARSING_CONTEXT[vNames_[i]].first == T_BOOL){
            z3::expr e_v = c_.bool_const(vNames_[i].c_str());            
            if(valuation[i] == 0){
                val_expr = val_expr && e_v == c_.bool_val(false);
            }else{
                val_expr = val_expr && e_v == c_.bool_val(true);
            }
        }else{
            z3::expr e_v = c_.int_const(vNames_[i].c_str());
            val_expr = val_expr && e_v == c_.int_val((__int64)valuation[i]);
        }
    }
    s.add((negation ? (!propExpr_[idx]) : propExpr_[idx])
          && limitsExpr_
          && val_expr);

    return s.check() == z3::sat;    
}

