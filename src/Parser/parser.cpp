#include <string>
#include <iostream>
#include <sstream>

#include <assert.h>

#include "parser.h"
#include "exceptions.h"



/** Overloading for printing ASTs. **/

std::ostream& operator<< (std::ostream& out, parser::AST const& ast){
    cout << "(" << parser::symTable[ast.s] << ", " << ast.n << ", <" 
         << ast.ln << "," << ast.cl << ">" << ", [";

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

/** Parser class implementation **/


/* @Parser class constructor.
*/ 
Parser::Parser(void){
    lexer       = new yyFlexLexer;
    pos         = -1;
    lastpos     = pos;
    skipws      = true;
}


/* @Parser class destructor.
*/ 
Parser::~Parser(){
    delete lexer;
}


/* @Get the line number for the lexeme at position @p in
    the lexemes vector.
   @p: the position of the lexeme to ask for. Should be valid
    position, i.e. >= 0 and < number of lexed lexemes. 
*/
int
Parser::getLineNum(int p){
    assert(p < static_cast<int>(tokens.size()));
    int result = 1;
    for(int i = 0; i < p; i++){
        if(tokens[i] == NL) result++;
    }
    return result;
}


/* @Get the column number for the starting position of lexeme at 
    position @p in the lexemes vector.
   @p: the position of the lexeme to ask for. Should be valid
    position, i.e. >= 0 and < number of lexed lexemes. 
*/
int 
Parser::getColumnNum(int p){
    assert(p < static_cast<int>(tokens.size()));
    int result = 1;

    for(int i = p-1; i >= 0 && tokens[i] != NL; i--){
        result += lexemes[i].size();
    }    

    return result;
}


/* @Get the next token from the tokens vector and make it
    available in @tkn class member. If @skipws then it will skip
    white space tokens and make available the next non white 
    one. If the end of the tokens vector is reached the @tkn
    will contain the MEOF token.
*/
void
Parser::nextLxm(void){

    assert(pos < static_cast<int>(tokens.size()));
    assert(pos < tokens.size()-1 || (Token)tokens[pos] == MEOF);

    pos++;
    tkn = (Token)tokens[pos];

    if( skipws && isw() ){      // do we skip whites?
        //lastpos++;              // FIXME should I accept this WS?
        nextLxm();
    }    
}


/* @Consume next token and check if it matches with @s.
   @return: 1 if it matches, 0 if it does not.
*/
int
Parser::accept(Token s){
    if (tkn == s) {
        lastpos = pos;  
        lastAcc = lexemes[pos];
        nextLxm();
        return 1;
    }
    return 0;    
}
 
/* @Consume the next token and trow an exception if it 
    does not match with @s.
   @s: the expected token.
   @return: 1 if s was matched.
*/
int
Parser::expect(Token s){
    if (accept(s))
        return 1;
    // TODO maybe the error report will be more clear if it was 
    //      thrown from the rule method and not here. It will
    //      be clearly more context specific.
    string ss = string("Expected a '") + string(symTable[s])
              + string("', got '") + lexemes[pos] 
              + string("' instead.\n");

    throw(new SyntaxError(ss, lines[pos], columns[pos]));
}



/** For looking ahead in the grammar. **/



/* @When grammar did not match, return to the saved state.
*/
int 
Parser::loadLocation(){
    pos = lastk.top();
    lastk.pop();
    tkn = (Token)tokens[pos];
}



/** Grammar Rules **/



/* @The starting point of the grammar to be parsed with the
    recursive descent parser.
   @return: 1 if successfully parsed. Throw SyntaxError otherwise.
*/
int
Parser::rGrammar(){

    newNode(_MODEL, string(""));

    while(!ended()){
        if(!rModule()){
            // Could not match the grammar. Show where we got stuck.
            throw (new SyntaxError(string("Syntax error after: '")
                                          + string(lexemes[lastpos])
                                          + string("'\n")
                                   ,getLineNum(lastpos)
                                   ,getColumnNum(lastpos)));
        }
    }
    return 1;
}


/* @Rule: MODULE */
int
Parser::rModule(){

    if(accept(KMOD)){                           // Should be
        // This node is a MODULE node
        newNode(_MODULE, string(""));//FIXME can give line and col from lastpos
        saveNode(_KEYWORD);
        // Get the modules name
        expect(NAME);                           // Has to be
        saveNode(_NAME);
        
        // while(rClkSec() || rVarSec() || rTranSec()) {;} FIXME
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
        newNode(_CLOCKSEC,string(""));
        saveNode(_KEYWORD);
        __debug__("Found clock section.\n");
        while(rClkDef()){;}
        saveNode(); // _CLOCKSEC
        return 1;
    }
    return 0;
}


/* @Rule: CLOCK DEFINITION */
int
Parser::rClkDef(){

    if (accept(NAME)){
        newNode(_CLOCK,string(""));
        saveNode(_NAME);
        if(accept(CLN)){
            saveNode(_SEPARATOR);
            if(rDistr()){
                expect(SCLN);
                saveNode(_SEPARATOR);
                saveNode(); //_CLOCK
                return 1;
            }
        }
        removeNode(); //_CLK
    }
    return 0;
}


/* @Rule: DISTRIBUTION */
int
Parser::rDistr(){
    if(accept(NAME)){
        newNode(_DISTRIBUTION, string(""));
        saveNode(_NAME);        // for now distributions are only names.
        saveNode(); // _DISTRIBUTION
        return 1;
    }
    return 0;
}


/* @Rule: MODULES VARIABLES SECTION */
int
Parser::rVarSec(){
    if(accept(KVS)){
        newNode(_VARSEC);
        saveNode(_KEYWORD);
        saveNode(); // _VARSEC
        __debug__("Found variables section.\n");
        return 1;
    }
    return 0;
}


/* @Rule: MODULES TRANSITIONS SECTION */
int
Parser::rTraSec(){
    if(accept(KTS)){
        newNode(_TRANSEC);
        saveNode(_KEYWORD);
        saveNode(); // _TRANSEC
        __debug__("Found transitions section.\n");
        return 1;
    }
    return 0;
}



/* @Parse: parse stream @str and place the resulting AST in @result.
           It is caller responsibility to free the allocated AST 
           @result afterwards.
   @return: 0 if something went wrong (print exception), 1 otherwise.
*/
int
Parser::parse(stringstream *str, AST * & result){

    int ret;
    int lineno = 1, colnum = 1;

    try{
        /* Lex */
        lexer->switch_streams((istream *)str);

        do{
            ret = lexer->yylex();

            lines.push_back(lineno);
            columns.push_back(colnum);

            if(ret){
                tokens.push_back(static_cast<Token>(ret));
                lexemes.push_back(lexer->YYText());
                if (ret == NL){
                    lineno++;
                    colnum = 1;
                }else{
                    colnum += lexer->YYLeng();
                }
            }else{
                tokens.push_back(MEOF);
                lexemes.push_back(string("EOF"));                
            }        
            
        }while(ret != 0);

        /* Parse */
        nextLxm();
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



/** AST construction methods **/


int
Parser::newNode(prodSym tkn,string str, int line, int col){
    
    Node *node = new Node(tkn,str,line,col);
    astStk.push(node);
}


int
Parser::newNode(prodSym tkn){
    
    Node *node = new Node(tkn,lexemes[lastpos],lines[lastpos],columns[lastpos]);
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
Parser::saveNode(prodSym tkn){
    newNode(tkn,lexemes[lastpos],lines[lastpos],columns[lastpos]);
    saveNode();
    return 1;
}


int
Parser::saveNode(prodSym tkn,string str, int line, int col){
    newNode(tkn,str,line,col);
    saveNode();
    return 1;
}


int
Parser::removeNode(){

    assert(!astStk.empty());
    delete astStk.top();
    astStk.pop();

}




/** Abstract syntax tree class implementation. **/

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

vector<AST*>
AST::get_list(prodSym k){

    vector<AST*> result;

    for(int i = 0; i < l.size(); i++){
        if (l[i]->s == k) result.push_back(l[i]);
    }

    return result;
}

}// namespace Parser
