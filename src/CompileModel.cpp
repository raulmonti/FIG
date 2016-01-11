/*
 */

#include "CompileModel.h"

namespace{

/**
 * TODO
 */
const int
solve(const AST* formula){

    //TODO
    int result = 0;

    return result;
}



/**
 * TODO
 **/
const State
CompileVars(const vector<AST*> varList)
{
    vector<VarDec> result;
    for(auto const &it: varList){
        string name = it->get_lexeme(_NAME);
        assert(name != "");
        vector<AST*> limits = {{0,0}};
        int init = 0;
        AST* ASTtype = it->get_first(_TYPE);
        AST* ASTrange = it->get_first(_RANGE);
        AST* ASTinit = it->get_first(_INIT);
        if(ASTrange){
            limits = range->get_list_lexemes(_NUM);
            assert(limits.size() == 2);
        }
        if(ASTinit){
            init = solve(ASTinit);
        }
        result.push_back(make_tuple(name, limits[0], limits[1], init));
    }
    return result;
}


/**
 * TODO
 **/
const vector<Clock>
CompileClocks(const vector<AST*> clockList)
{

    const vector<Clock> result;
    for(const auto &it: clockList){
        
    }
}


/**
 * TODO
 **/
std::shared_ptr<ModuleInstance> 
CompileModule(const AST* module)
{
    shared_ptr<ModuleInstance> result;
    
    vector<AST*> variables = module->get_all_ast(_VARIABLE);
    vector<AST*> clocks = module->get_all_ast(_CLOCK);

    result = make_shared<Module>(name
                                ,CompileVars(variables)
                                ,CompileClocks(clocks));

    //TODO transitions
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
CompileModel(const AST* astModel){
   
	auto model = fig::ModelSuite::get_instance();
	assert(!model.sealed());

    vector<AST*> modules = astModel->get_all_ast(_MODULE);
    for(auto const &it: modules){
        model.add_module(CompileModule(it));
    }
    // TODO seal
}

} // namespace fig


