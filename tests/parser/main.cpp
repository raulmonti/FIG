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
#include "config.h"

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
        }catch(const FigException &e){
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
    }catch(const FigException &e){
        cout << "[TEST] " << filename << " passed." << endl;
        cout << "======================\n" << e.what() 
             << "======================\n" << endl;
        return;
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
    }catch(const FigException &e){
        cout << "[TEST] " << filename << " passed." << endl;
        cout << "======================\n" << e.what() 
             << "======================\n" << endl;
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
        cout << "[TEST] " << filename << " passed.\n\n";
    }catch(const std::exception &e){
        cout << "[TEST] " << filename << " NOT passed!!\n" << endl;
        cout << "======================\n" << e.what() 
             << "======================\n" << endl;
        return;
    }


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
        cout << "[TEST] " << filename << " passed.\n\n";
    }catch(const std::exception &e){
        cout << "[TEST] " << filename << " NOT passed!!" << endl;
        cout << "======================\n" << e.what() 
        << "======================\n" << endl;
    }
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
        cout << "[TEST] " << filename << " passed.\n\n";
    }catch(const std::exception &e){
        cout << "[TEST] " << filename << " NOT passed!!\n";
        cout << "======================\n" << e.what() 
        << "======================\n\n";
    }

}

//==============================================================================
void
test_tandem_queue(string path)
{
    string filename = path.substr( path.find_last_of('/') + 1
                                 , string::npos);
    cout << "[TEST] " << filename << "..." << endl;
    try{
        compile(path);
        cout << "[TEST] " << filename << " passed." << endl;
    }catch(const std::exception &e){
        cout << "[TEST] " << filename << " NOT passed!!\n";
        cout << "======================\n" << e.what() 
        << "======================\n\n";
    }
    return;
    
}
//==============================================================================
int 
main (int argc, char** argv){

    char name[4096];
    realpath("tests/parser/models",name);
    string TestModelsPath(name);
    realpath("models",name);
    string CarlosModelsPath(name);

//    test_names(modelsPath + "/counterNames.sa");
//    test_iosa_condition_1_2(modelsPath + "/counterProp1y2.sa");
//    test_iosa_condition_3(modelsPath + "/counterProp3.sa");
//    test_iosa_condition_4(modelsPath + "/counterProp4.sa");
//    test_iosa_condition_7(modelsPath + "/counterProp7.sa");
    tout << "[TEST] ****** TESTING FIG EXAMPLES ******\n\n"; 
    test_tandem_queue(CarlosModelsPath + "/tandem_queue.sa");

    return 0;
}

