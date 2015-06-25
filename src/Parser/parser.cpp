#include <string>
#include <iostream>
#include <sstream>

#include <assert.h>

#include "parser.h"
#include "exceptions.h"


using namespace std;


namespace parser{

/*** Parser class implementation ***/

/* @Parser class constructor.
*/ 
Parser::Parser(void){
    lexer = new yyFlexLexer;
    pos   = -1;
    lastpos = pos;
    skipws = true;
}


/* @Parser class destructor.
*/ 
Parser::~Parser(){
    delete lexer;
}

/* @Get the line number for the word at position @p in
    the lexed words.
   @p: the position of the word to ask for. Should be valid
    position, i.e. >= 0 and < number of words lexed. 
*/
int
Parser::getLineNum(int p){
    assert(p < static_cast<int>(symvec.size()));
    int result = 0;
    for(int i = 0; i < p; i++){
        if(symvec[i] == NL) result++;
    }
    return result;
}


/* @Get the next symbol from the lexed vector and make it
    available in @sym class member. If @skipws then will skip
    white space symbols and make available the next non white 
    one. If the end of the lexed symbols is reached the @sym
    will contain the MEOF symbol.
*/
void
Parser::nextSym(void){

    assert(pos < static_cast<int>(symvec.size()));
    assert(pos < symvec.size()-1 || (Symbol)symvec[pos] == MEOF);

    pos++;
    sym = (Symbol)symvec[pos];

    if( skipws && isw() ){ // do we skip whites?
        lastpos++;         // FIXME should I accept this WS?
        nextSym();
    }    
}


/* @Consume next symbol and check if it matches with @s.
   @return: 1 if it matches, 0 if it does not.
*/
int
Parser::accept(Symbol s){
    if (sym == s) {
        lastpos = pos;  
        nextSym();
        return 1;
    }
    return 0;    
}
 
/* @Consume the next symbol and trow an exception if it 
    does not match with @s.
   @s: the expected symbol.
   @return: 1 if s whas matched.
*/
int
Parser::expect(Symbol s){
    if (accept(s))
        return 1;
    string ss = string("Expected ")   + string(symTable[s]) 
              + string(" type, got ") + strvec[pos] 
              + string(", of type ")  + string(symTable[sym]);
    throw(new SyntaxError(ss));
}

/*** For looking ahead in the grammar. ***/



/* @When grammar did not match, return to the saved state.
*/
int 
Parser::loadLocation(){
    pos = lastk.top();
    lastk.pop();
    sym = (Symbol)symvec[pos];
}


/*** Grammar Rules ***/

/* @The starting point of the grammar to be parsed with the
    recurive descent parser.
   @return: ...
*/
int 
Parser::grammar(){

    while(!ended()){
        if(!rModule()) {
            throw (new SyntaxError((string("Syntax error at line ")) 
                        + to_string(getLineNum(lastpos+1)) + string(": '") 
                        + strvec[lastpos+1]+string("'")));
        }
    }
    return 1;
}

/* @Rule: MODULE */
int
Parser::rModule(){

    if(accept(KMOD)){
        expect(NAME); //the module name
        rClkSec();
        rVarSec();
        rTraSec();
        cout << "Found Module." << endl;
        return 1;
    }
    return 0;
}

/* @Rule: MODULES CLOCKS SECTION */
int
Parser::rClkSec(){
    if(accept(KCS)){
        cout << "Found clock section." << endl;
        return 1;
    }
    return 0;
}

/* @Rule: MODULES VARIABLES SECTION */
int
Parser::rVarSec(){
    if(accept(KVS)){
        cout << "Found variables section." << endl;
        return 1;
    }
    return 0;
}

/* @Rule: MODULES TRANSITIONS SECTION */
int
Parser::rTraSec(){
    if(accept(KTS)){
        cout << "Found transitions section." << endl;
        return 1;
    }
    return 0;
}


/* @Parse ...
   @return: ...
*/
int
Parser::parse(stringstream *str){

    try{
        /* Lex */
        setStream(str);

        int ret;
        do{
            ret = lexer->yylex();
            symvec.push_back(static_cast<Symbol>(ret));
            strvec.push_back(lexer->YYText());
        }while(ret != 0);

        /* Parse */
        nextSym();
        grammar();

    }catch(exception *e){
        cout << "Parser: " << e->what() << endl;
        delete e;
        return 0;
    }

    return 1;
}


}
