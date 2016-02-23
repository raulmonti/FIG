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
#include "ReduceProperty.h"
#include "Parser.h"

using namespace std;
using namespace parser;


//==============================================================================
AST*
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
    return pp.first;
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

void
test_parse_properties(string modelpath, string proppath){

    string modelfilename = modelpath.substr( modelpath.find_last_of('/') + 1
                                 , string::npos);
    string propfilename = proppath.substr( proppath.find_last_of('/') + 1
                                 , string::npos);
    tout << "[TEST] " << modelfilename << ", " << propfilename << "..." << endl;
    AST* model = compile(modelpath);
    assert(model);
    
    auto parser = Parser();
    /* Get a stream with the model to parse. */
    ifstream fin(proppath,ios::binary);
    stringstream ss;
    ss << fin.rdbuf();
    /* Parse. */
    AST* ast = parser.parseProperties(& ss);

    /* Compile into simulation model */
    CompileModel(GLOBAL_MODEL_AST, GLOBAL_PARSING_CONTEXT);

    if(ast){
        vector<AST*> properties = ast->get_all_ast(_PROPERTY);
        vector<AST*> modules = model->get_all_ast(_MODULE);
        for(int i = 0; i < properties.size(); ++i){
            for(const auto &it: modules){
                string mname = it->get_lexeme(_NAME);
                try{
                    tout << "(" << i << ")" << ", " << mname << ": " <<
                        reduceProperty(i,mname) << endl;
                }catch(const FigException &e){
                    tout << e.what() << endl;                
                }
            }
        }        
    }
}

//==============================================================================
int 
main (int argc, char** argv){

    char name[4096];
    realpath("tests/parser/models",name);
    string TestModelsPath(name);
    realpath("models",name);
    string CarlosModelsPath(name);
    realpath("tests/parser/properties",name);
    string PropertiesPath(name);


//    test_names(TestModelsPath + "/counterNames.sa");
//    test_iosa_condition_1_2(TestModelsPath + "/counterProp1y2.sa");
//    test_iosa_condition_3(TestModelsPath + "/counterProp3.sa");
//    test_iosa_condition_4(TestModelsPath + "/counterProp4.sa");
//    test_iosa_condition_7(TestModelsPath + "/counterProp7.sa");

    tout << "[TEST] ****** TESTING FIG EXAMPLES ******\n\n"; 
    test_tandem_queue(CarlosModelsPath + "/tandem_queue.sa");
    tout << endl;

    tout << "[TEST] ****** TESTING PROPERTIES ******\n\n";
    test_parse_properties(CarlosModelsPath + "/tandem_queue.sa"
        , PropertiesPath + "/dummy_props.sa");

    return 0;
}

