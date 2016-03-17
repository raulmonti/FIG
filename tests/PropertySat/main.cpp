//==============================================================================
//
//  Example of parsing and compiling a IOSA model into a FIG
//  simulation model.
//  Raul Monti
//  2016
//  FIG project.
//
//==============================================================================

#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <utility> // pair
#include "Parser.h"
#include "PreCompiler.h"
#include "Iosacompliance.h"
#include "CompileModel.h"
#include "PropertySat.h"


using namespace std;

pair<int,int>
get_var_range(AST *ast, parsingContext &pc)
{
    assert(ast->tkn == _VARIABLE);

    pair<int,int> result;
    string name = ast->get_lexeme(_NAME);
    if(pc[name].first == T_BOOL){
        result = make_pair(0,1);
    }else{
        AST* rangeAST = ast->get_first(_RANGE);
        vector<AST*> limitsASTv = rangeAST->get_all_ast(_EXPRESSION);
        assert(limitsASTv.size() == 2);
        result = make_pair(
            stoi(solve_const_expr(limitsASTv[0],GLOBAL_PARSING_CONTEXT)),
            stoi(solve_const_expr(limitsASTv[1],GLOBAL_PARSING_CONTEXT)));
    }
    return result;
}

//==============================================================================

void
CheckPropsSat(void)
{
    assert(GLOBAL_MODEL_AST);
    assert(GLOBAL_PROP_AST);


    vector<AST*> props = GLOBAL_PROP_AST->get_all_ast(_PROPERTY);
    vector<AST*> modules = GLOBAL_MODEL_AST->get_all_ast(_MODULE);
    size_t count  = GLOBAL_PROP_AST->get_all_ast(_PROPERTY).size();
    map<string,pair<int,int>> ranges;
    for(const auto &it: modules){
        // get this modules variables and their ranges
        vector<AST*> vars_ast = it->get_all_ast(_VARIABLE);
        vector<string> vars;
        for(const auto &it2: vars_ast){
            string vname = it2->get_lexeme(_NAME);
            vars.push_back(vname);
            ranges[vname] = get_var_range(it2, GLOBAL_PARSING_CONTEXT);
        }
        // check each property for this module
        for(size_t i = 0; i < count; ++i){
            fig::PropertySat ps(i,vars);
            // for each posible module state
            size_t numStates = 1;
            for(size_t j = 0; j < vars.size(); ++j){
                numStates *= ranges[vars[j]].second-ranges[vars[j]].first+1;
            }
            for(size_t n = 0; n < numStates; ++n){
                vector<short> valuation;
                for(size_t j = 0; j < vars.size(); ++j){
                    size_t stride = 1;
                    for(size_t k = j+1; k < vars.size(); ++k){
                        stride *= ranges[vars[k]].second-ranges[vars[k]].first+1;
                    }
                    valuation.push_back((ranges[vars[j]].first + (n/stride))
                        % (ranges[vars[j]].second-ranges[vars[j]].first+1));
                }
 
                bool ret = ps.sat(0,valuation);

                tout << "Prop " << props[i]->toString() << ", State [";
                for(int i = 0; i < vars.size()-1 ;++i){
                    tout << vars[i] << "=" << valuation[i] << ",";
                }
                tout << vars[vars.size()-1] << "=" << valuation[vars.size()-1]
                    << "] -> " << ret << endl;
            }
        }
        
    }

}

//==============================================================================

int main (int argc, char** argv){

    assert(argc == 3 && "Use: ./FigTest <modelFileName> <propertiesFileName>");

    tout << "Model file: "      << argv[1] << endl;
    tout << "Properties file: " << argv[2] << endl;

    auto parser         = parser::Parser();
    auto verifier       = parser::Verifier(); 
    auto precompiler    = parser::Precompiler();

    ifstream mfin(argv[1],ios::binary);
    stringstream ss;
    ss << mfin.rdbuf();

    parser.parse(&ss);  
    ss.str("");
    ss.clear();
    ss << precompiler.pre_compile(GLOBAL_MODEL_AST,GLOBAL_PARSING_CONTEXT);
    parser.parse(&ss);
    verifier.verify(GLOBAL_MODEL_AST,GLOBAL_PARSING_CONTEXT);

    ifstream pfin(argv[2],ios::binary);    
    ss.str("");
    ss.clear();
    ss << pfin.rdbuf();
    parser.parseProperties(&ss);
    ss.str("");
    ss.clear();
    cout << ss.str() << endl;
    ss << precompiler.pre_compile_props();
    cout << ss.str() << endl;
    parser.parseProperties(&ss);

    /* Compile into simulation model */
    fig::CompileModel(GLOBAL_MODEL_AST, GLOBAL_PARSING_CONTEXT);

    /* Sat for properties */
    CheckPropsSat();
    


    return 0;
}

