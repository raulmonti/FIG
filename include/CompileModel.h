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


using namespace std;
using namespace parser;

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

