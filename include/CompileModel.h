/*
*/

#ifndef COMPILE_MODEL_H
#define COMPILE_MODEL_H

#include <vector>
#include "Ast.h"
#include "ModuleInstance.h"
#include "Clock.h"
#include "State.h"
#include "Parser.h"

using parser::parsingContext;


typedef fig::State<fig::STATE_INTERNAL_TYPE>                state;
typedef fig::VariableDefinition<fig::STATE_INTERNAL_TYPE>   varDec;

namespace fig{

/**
 * @brief Compile the model in the AST to a FIG simulation model.
 * @param [in] astModel A valid IOSA model.
 * @rise ...
 */
void
CompileModel(AST* astModel, const parsingContext &pc);
   

}

#endif

