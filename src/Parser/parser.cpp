#include <string>
#include <iostream>
#include <sstream>

#include <assert.h>

#include "parser.h"
#include "exceptions.h"



/*** Overloading for printing ASTs. ***/
std::ostream& operator<< (std::ostream& out, parser::AST const& ast){
    cout << "(" << ast.s << ", " << ast.n << ", [";

    for(size_t i = 0; i+1 < (ast.l).size(); i++){
        parser::AST *a = (ast.l)[i]; 
        out << *a << ",";
    }

    if(ast.l.size()>0){
        parser::AST *a = ast.l[(ast.l).size()-1];
        out << *a;
    }
    cout << "])";
    return out;
}





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
    int lineno = 1;
    int colnum = 1;
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
    int result = 1;
    for(int i = 0; i < p; i++){
        if(symvec[i] == NL) result++;
    }
    return result;
}


/* @Get the column number for the starting position of word at 
    position @p in the lexed words.
   @p: the position of the word to ask for. Should be valid
    position, i.e. >= 0 and < number of words lexed. 
*/
int 
Parser::getColumnNum(int p){
    assert(p < static_cast<int>(symvec.size()));
    int result = 1;

    for(int i = p-1; i >= 0 && symvec[i] != NL; i--){
        result += strvec[i].size();
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

    if( skipws && isw() ){      // do we skip whites?
        lastpos++;              // FIXME should I accept this WS?
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
        lastAcc = strvec[pos];
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
Parser::rGrammar(){

    newNode(_DUM,string("GRAMMAR"));

    while(!ended()){
        if(!rModule()){
            throw (new SyntaxError((string("Syntax error at line "))
                        + to_string(getLineNum(lastpos+1))
                        + string(", and column ")
                        + to_string (getColumnNum(lastpos+1))
                        + string(": '")
                        + strvec[lastpos+1]
                        + string("'")));
        }
    }
    return 1;
}

/* @Rule: MODULE */
int
Parser::rModule(){

    if(accept(KMOD)){                           // Should be
        // This node is a MODULE node
        newNode(_KMOD,lastAcc,lineno,colnum);
        // Get the modules name
        expect(NAME);                           // Has to be
        saveNode(_NM,lastAcc,lineno,colnum);
        
        rClkSec();                              // ?
        rVarSec();                              // ?
        rTraSec();                              // ?

        __debug__("Found Module.\n");
        saveNode();
        return 1;
    }
    return 0;
}

/* @Rule: MODULES CLOCKS SECTION */
int
Parser::rClkSec(){
    if(accept(KCS)){
        saveNode(_KCS,lastAcc,lineno,colnum);
        __debug__("Found clock section.\n");
        while(rClkDef()){;}       
        return 1;
    }
    return 0;
}

/* @Rule: CLOCK DEFINITION */
int
Parser::rClkDef(){

    newNode(_CLK,string(""),lineno,colnum);
    if (accept(NAME)){
        saveNode(_NM, lastAcc,lineno,colnum);
        if(accept(CLN)){
            saveNode(_CLN, lastAcc,lineno,colnum);
            if(rDistr()){
                expect(SCLN);
                saveNode(_SCLN, lastAcc,lineno,colnum);
                saveNode(); //_CLK
                return 1;
            }
        }
    }
    removeNode(); //_CLK
    return 0;
}

/* @Rule: DISTRIBUTION */
int
Parser::rDistr(){
    if(accept(NAME)){
        saveNode(_DSTR, string(lastAcc),lineno,colnum);
        return 1;
    }
    return 0;
}


/* @Rule: MODULES VARIABLES SECTION */
int
Parser::rVarSec(){
    if(accept(KVS)){
        saveNode(_KCS,lastAcc,lineno,colnum);
        __debug__("Found variables section.\n");
        return 1;
    }
    return 0;
}

/* @Rule: MODULES TRANSITIONS SECTION */
int
Parser::rTraSec(){
    if(accept(KTS)){
        saveNode(_KCS,lastAcc,lineno,colnum);
        __debug__("Found transitions section.\n");
        return 1;
    }
    return 0;
}



/* @Parse ...
   @return: ...
*/
int
Parser::parse(stringstream *str, AST * & result){

    try{
        /* Lex */
        lexer->switch_streams((istream *)str);

        int ret;
        do{
            ret = lexer->yylex();
            symvec.push_back(static_cast<Symbol>(ret));
            strvec.push_back(lexer->YYText());

            if (ret == NL){
                lineno++;
                colnum = 0;
            }else{
                colnum += lexer->YYLeng();
            }   
            
        }while(ret != 0);

        /* Parse */
        nextSym();
        if (rGrammar()){
            result = astStk.top();
        }

    }catch(exception *e){
        cout << "Parser: " << e->what() << endl;
        delete e;
        return 0;
    }

    return 1;
}





int
Parser::newNode(prodSym sym,string str, int line, int col){
    
    Node *node = new Node(sym,str,line,col);
    astStk.push(node);
}

int
Parser::saveNode(){

    assert(!astStk.empty());

    // Pop it from the stack.
    Node *temp =  astStk.top();
    astStk.pop();

    // And save it to the recursive AST.
    if (!astStk.empty()){
        astStk.top()->pb(temp);        
    }
}


int
Parser::saveNode(prodSym sym,string str, int line, int col){
    newNode(sym,str,line,col);
    saveNode();
    return 1;
}


int
Parser::removeNode(){

    assert(!astStk.empty());
    delete astStk.top();
    astStk.pop();

}


/*** Abstract syntax tree class implementation. ***/

AST::AST(){};

AST::AST(int symbol, string name, int line, int col): 
    n(name),
    s(symbol),
    ln(line),
    cl(col)
{};


AST::~AST(){
    for( int i = 0; i < l.size(); i++){
        delete l[i];
        l[i] = NULL;
    }
}

}
