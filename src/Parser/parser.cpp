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
    if (!lookahead()) {
        throw (new SyntaxError((string("Syntax error at line ")) 
                  + to_string(getLineNum(lastpos+1)) + string(": '") 
                  + strvec[lastpos+1]+string("'")));
    }
    return 1;
}

/* @Rule: transition section. */
int 
Parser::lookahead(){
    if(output()){
        ;
    }else if(input()){
        ;
    }else{
        return 0;
    }
    return 1;
}

/* @Rule: output transition. */
int
Parser::output(){
    saveLocation();
    if (accept(NUM)){
        accept(WS);
        if (accept(NUM)){
            accept(WS);
            expect(NAME);
            removeLocation();
            cout << "OUTPUT" << endl;
            return 1;
        }
    }
    loadLocation();
    return 0;
}

/* @Rule: input transition. */
int
Parser::input(){
    saveLocation();
    if (accept(NUM)){
        accept(WS);
        if (accept(NAME)){
            accept(WS);
            expect(NUM);
            removeLocation();
            cout << "INPUT" << endl;
            return 1;            
        }
    }
    loadLocation();
    return 0;
}


/* @Parse ...
   @return: ...
*/
int
Parser::parse(string str){


    try{
        /* Lex */
        stringstream *ss = new stringstream();
        *ss << str;
        setStream(ss);

        int ret;
        do{
            ret = lexer->yylex();
            symvec.push_back(static_cast<Symbol>(ret));
            strvec.push_back(lexer->YYText());
            cout << "Parser: Symbol: " << ret << endl;
        }while(ret != 0);    

        delete ss;

        /* Parse */
        nextSym();
        while(!ended()){
            grammar();
        }
    }catch(exception *e){
        cout << "Parser: " << e->what() << endl;
        return 0;
    }

    return 1;
}


}
