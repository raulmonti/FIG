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
#include "Parser.h"
#include "PreCompiler.h"
#include "Iosacompliance.h"
#include "CompileModel.h"


using namespace std;



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
    ss.clear();
    ss << precompiler.pre_compile(GLOBAL_MODEL_AST,GLOBAL_PARSING_CONTEXT);
    parser.parse(&ss);
    verifier.verify(GLOBAL_MODEL_AST,GLOBAL_PARSING_CONTEXT);

    ifstream pfin(argv[2],ios::binary);    
    ss.clear();
    ss << pfin.rdbuf();
    parser.parseProperties(& ss);

    /* Compile into simulation model */
    fig::CompileModel(GLOBAL_MODEL_AST, GLOBAL_PARSING_CONTEXT);

    return 0;
}

