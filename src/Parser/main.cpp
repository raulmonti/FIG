#include <iostream>
#include <string>
#include "parser.h"

using namespace std;

 


int 
main (int /* argc */, char** /* argv */){

    cout << "RUNNING THE PARSER" << endl << endl;

    string str2parse;
    cout << "Write what do you want to parse:" << endl;
    getline(cin, str2parse);

    str2parse = string("\n\n 2 3 e \n w w e\n");

    cout << "Parsing: " << str2parse << endl << endl;

    parser::Parser *parser = new parser::Parser();
    parser->parse(str2parse);
    delete parser;

    return 0;
}
