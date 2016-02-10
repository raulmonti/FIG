/*
*/

#ifndef COMPILE_MODEL_H
#define COMPILE_MODEL_H

#include <vector>
#include "ast.h"
#include "ModuleInstance.h"
#include "Clock.h"
#include "State.h"
#include "parser.h"

using namespace fig;
using namespace std;
using namespace parser;


typedef fig::State<fig::STATE_INTERNAL_TYPE>                state;
typedef fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>   varDec;
typedef std::list< std::string >                            NamesList;

namespace fig{

/**
 * @brief Compile the model in the AST to a FIG simulation model.
 * @param [in] astModel A valid IOSA model.
 * @rise ...
 */
void
CompileModel(AST* astModel, const parsingContext &pc);
   

}

namespace{

/**
 *
 */
const state
CompileVars(const vector<AST*> varList  );


/**
 *
 */
const vector<Clock>
CompileClocks(const vector<AST*> transitions);


/**
 *
 */
fig::Transition
CompileTransition(AST* trans);


/**
 *
 */
std::shared_ptr<ModuleInstance> 
CompileModule(AST* module);

} //namespace

#endif

