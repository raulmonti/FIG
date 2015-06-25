#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <assert.h>
#include "parser.h"


using namespace std;

 


int 
main (int argc, char** argv){

    assert( argc == 2);

    cout << "RUNNING THE PARSER" << endl << endl;
    
    cout << "Parsing file: " << argv[1] << endl << endl;

    parser::Parser *parser = new parser::Parser();

    ifstream fin(argv[1],ios::binary);
    stringstream ss;
    ss << fin.rdbuf();

    parser->parse(& ss);
    delete parser;

    return 0;
}
