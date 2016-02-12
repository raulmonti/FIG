// C++
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
// FIG
#include <ModuleNetwork.h>
#include "Parser.h"
#include "Ast.h"
#include <exception>
#include "Exceptions.h"
#include "CompileModel.h"
#include "PreCompiler.h"


int main(int argc, char** argv)
{
	std::cout << "Hello FIG!" << std::endl;
    assert(argc > 1);

    Parser      parser      = Parser();
    Verifier    verifier    = Verifier();
    Precompiler precompiler = Precompiler();

    /* read the model */
    ifstream fin(filename,ios::binary);
    stringstream ss;
    ss << fin.rdbuf();
    /* parse the model */
    pair<AST*, parsingContext> pp = parser.parse(& ss);
    if(pp.first){
        try{
            stringstream pss;
            /* solve constants (precompile) */
            pss << precompiler.pre_compile(pp.first,pp.second);
            delete pp.first;
            /* parse again (with solved constants) */
            pp = parser.parse(&pss);
            /* verify IOSA compliance and other stuff */
            verifier.verify(pp.first,pp.second);
            /* compile to a simulation model */
            fig::CompileModel(pp.first,pp.second);
        }catch(FigException &e){
            delete pp.first;   
            throw e;
        }
    }
    /** TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO **/
    /** HERE WE SHOULD SIMULATE AND DO ALL THE STUFF CARLOS KNOWS ABOUT. **/

    /* Free the parsed model */
    delete pp.first;
	return 0;
}
