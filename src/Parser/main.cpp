#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <assert.h>
#include <stdio.h>
#include "parser.h"


using namespace std;




int 
main (int argc, char** argv){

    assert( argc == 2);

    cout << ">> Running the parser ..." << endl;
    
    cout << ">> Parsing file: " << argv[1] << endl ;

    parser::Parser *parser = new parser::Parser();

    ifstream fin(argv[1],ios::binary);
    stringstream ss;
    ss << fin.rdbuf();

    parser::AST * ast;
    if(parser->parse(& ss, ast)){
        cout << ">> Result of Parsing:\n\n" <<  *ast << endl << endl;
    }

    delete parser;
    delete ast;

    return 0;
}
