#include <vector>
#include <string>
#include "parsingContext.h"


using namespace std;


namespace parser{


parsingContext::parsingContext(){

}


parsingContext::parsingContext(const parsingContext & pc){

    typeMap = pc.typeMap;
    clckMap = pc.clckMap;

}

parsingContext::~parsingContext(){

}


/*
*/
bool
parsingContext::add_var(string module, string name, Type t){
    typeMap[module][name] = t;
    return 1;
}

/*
*/
bool
parsingContext::add_clock(string module, string c){
    clckMap[module].insert(c);
    return 1;
}


/*
*/
bool
parsingContext::has_var(string module, string v){
    return typeMap[module].count(v) > 0;    
}

/*
*/
parser::Type
parsingContext::get_var_type(string module, string name){
    return (*(typeMap[module].find(name))).second;
}

/*
*/
bool
parsingContext::has_clock(string module, string c){
    return clckMap[module].count(c) > 0;
}

}// namespace parser
