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
#include "Parser.h"
#include "Ast.h"
#include "Iosacompliance.h"
#include <exception>
#include "Exceptions.h"
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
    auto parser   = Parser();
    auto prec     = Precompiler();
    auto verifier = Verifier();

    /* Get a stream with the model to parse. */
    ifstream fin(filename,ios::binary);
    stringstream ss;
    ss << fin.rdbuf();
    /* Parse. */
    pair<AST*, parsingContext> pp = parser.parse(& ss);
    /* Verify. */
    if(pp.first){
        __debug__("[DEBUG] Result of Parsing:\n\n");
        __debug__(pp.first);
        __debug__("[DEBUG]\n");

        try{
            stringstream pss;
            pss << prec.pre_compile(pp.first,pp.second);
            parser.clear();
            pp = parser.parse(&pss);
            verifier.verify(pp.first,pp.second);
            fig::CompileModel(pp.first,pp.second);
        }catch(FigException &e){
            delete pp.first;   
            throw e;
        }
    }

    /* We are in charge of deleting the AST. */
    delete pp.first;
}

//==============================================================================
void
test_names(string path)
{
    string filename = path.substr( path.find_last_of('/') + 1
                                 , string::npos);
    cout << "[TEST] " << filename << "..." << endl;
    try{
        compile(path);
    }catch(FigException &e){
        cout << "[TEST] " << filename << " passed." << endl;
        cout << "======================\n" << e.msg() 
             << "\n======================\n" << endl;
        return;
    }catch(...){
        assert(false);
    }
    cout << "[TEST] " << filename << " NOT passed!!" << endl;
}

//==============================================================================
void
test_iosa_condition_1_2(string path)
{
    string filename = path.substr( path.find_last_of('/') + 1
                                 , string::npos);
    cout << "[TEST] " << filename << "..." << endl;
    try{
        compile(path);
    }catch(FigException &e){
        cout << "[TEST] " << filename << " passed." << endl;
        cout << "======================\n" << e.msg() 
             << "\n======================\n" << endl;
        return;
    }catch(...){
        assert(false);
    }
    cout << "[TEST] " << filename << " NOT passed!!" << endl;
}

//==============================================================================
void
test_iosa_condition_3(string path)
{
    string filename = path.substr( path.find_last_of('/') + 1
                                 , string::npos);
    cout << "[TEST] " << filename << "..." << endl;
    try{
        compile(path);
    }catch(FigException &e){
        cout << "[TEST] " << filename << " passed." << endl;
        cout << "======================\n" << e.msg() 
             << "\n======================\n" << endl;
        return;
    }catch(...){
        assert(false);
    }
    cout << "[TEST] " << filename << "NOT passed!!" << endl;
}

//==============================================================================
void
test_iosa_condition_4(string path)
{
    string filename = path.substr( path.find_last_of('/') + 1
                                 , string::npos);
    cout << "[TEST] " << filename << "..." << endl;
    try{
        compile(path);
    }catch(FigException &e){
        cout << "[TEST] " << filename << " passed." << endl;
        cout << "======================\n" << e.msg() 
             << "\n======================\n" << endl;
        return;
    }catch(...){
        assert(false);
    }
    cout << "[TEST] " << filename << "NOT passed!!" << endl;
}

//==============================================================================
void
test_iosa_condition_7(string path)
{
    string filename = path.substr( path.find_last_of('/') + 1
                                 , string::npos);
    cout << "[TEST] " << filename << "..." << endl;
    try{
        compile(path);
    }catch(FigException &e){
        cout << "[TEST] " << filename << " passed." << endl;
        cout << "======================\n" << e.msg() 
             << "\n======================\n" << endl;
        return;
    }catch(...){
        assert(false);
    }
    cout << "[TEST] " << filename << "NOT passed!!" << endl;
}

//==============================================================================
/*int 
main (int argc, char** argv){

    char name[4096];
    realpath("tests/parser/models",name);
    string modelsPath(name);

    test_names(modelsPath + "/counterNames.sa");
    test_iosa_condition_1_2(modelsPath + "/counterProp1y2.sa");
    test_iosa_condition_3(modelsPath + "/counterProp3.sa");
    test_iosa_condition_4(modelsPath + "/counterProp4.sa");
    test_iosa_condition_7(modelsPath + "/counterProp7.sa");

    return 0;
}*/



int main(int argc, char** argv)
{
	std::cout << "Hello FIG!" << std::endl;
    assert(argc > 1);

    Parser      parser      = Parser();
    Verifier    verifier    = Verifier();
    Precompiler precompiler = Precompiler();

    /* read the model */
    ifstream fin(argv[1],ios::binary);
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
    /* Free the parsed model */
    delete pp.first;
	return 0;
}


