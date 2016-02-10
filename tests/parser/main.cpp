//==============================================================================
//
//  Running example for the parser.
//  Raul Monti
//  2015
//  FIG project.
//
//==============================================================================

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <assert.h>
#include <stdio.h>
#include "parser.h"
#include "ast.h"
#include "iosacompliance.h"
#include <exception>
#include "exceptions.h"
#include "CompileModel.h"
#include "PreCompiler.h"
#include <stdlib.h>

using namespace std;
using namespace parser;

//==============================================================================




void
compile(string filename)
{
    //FIXME check that filename is valid

    auto parser   = new Parser();
    auto prec     = new Precompiler();
    auto verifier = new Verifier();

    /* Get a stream with the model to parse. */
    ifstream fin(filename,ios::binary);
    stringstream ss;
    ss << fin.rdbuf();
    /* Parse. */
    pair<AST*, parsingContext> pp = parser->parse(& ss);
    /* Verify. */
    cout << filename << "\n" << *pp.first << endl;
    if(pp.first){
        __debug__("[DEBUG] Result of Parsing:\n\n");
        __debug__(pp.first);
        __debug__("[DEBUG]\n");

        try{
            stringstream pss;
            pss << prec->pre_compile(pp.first,pp.second);
            parser->clear();
            pp = parser->parse(&pss);
            verifier->verify(pp.first,pp.second);
            fig::CompileModel(pp.first,pp.second);
        }catch(std::exception &e){
            delete pp.first;
            delete parser;
            delete verifier;
            delete prec;      
            throw e;
        }catch(string &e){
            delete pp.first;        
            delete parser;
            delete verifier;
            delete prec;
            throw e;
        }
    }

    /* We are in charge of deleting the AST. */
    delete parser;
    delete verifier;
    delete prec;
    delete pp.first;
}

void
test_names(string filename)
{
    cout << "[TESTING] NAMES ..." << endl;
    try{
        compile(filename);
    }catch(std::exception e){
        cout << e.what() << endl;
        assert(false);
    }
    cout << "[TESTING] NAMES NOT PASSED" << endl;
}


int 
main (int argc, char** argv){

    char name[4096];
    realpath("tests/parser/models/counterNames.sa",name);
    string modelsPath(name);

    test_names(modelsPath + "/counterNames.sa");
    return 0;
}






