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


using namespace std;
using namespace parser;


int 
main (int argc, char** argv){

    assert( argc == 2);

    cout << ">> Running the parser ..." << endl;
    cout << ">> Parsing file: " << argv[1] << endl ;

    /* Instanciate a parser, and a verifier. */
    parser::Parser *parser = new parser::Parser();
    parsingContext pc;
    Verifier *verifier = new Verifier(pc);

    /* Get a stream with the model to parse. */
    ifstream fin(argv[1],ios::binary);
    stringstream ss;
    ss << fin.rdbuf();

    /* Parse and point to the resulting AST. */
    AST * ast = NULL;
    if(parser->parse(& ss, ast)){
        __debug__(">> Result of Parsing:\n\n");
        __debug__(*ast);
        __debug__("\n\n");

        /* Do something with the resulting AST ... 
           like printing each modules name: */
        vector<AST*> modules = ast->get_list(parser::_MODULE);
        for (int i = 0; i < modules.size(); i++){
            //cout << (modules[i])->branches[1]->lxm << endl;
            //OR
            //cout << (modules[i])->get_list(parser::_NAME)[0]->lxm << endl; 
            //OR
            //cout << modules[i]->get_list_lexemes(parser::_NAME)[0] << endl;
        }

        try{
            verifier->verify(ast);

        }catch(string e){

            cout << e << endl;
        }

        /* We are in charge of deleting the AST. */
        if (ast != NULL){
            delete ast;
        }
    }

    delete parser;
    delete verifier;

    return 0;
}

