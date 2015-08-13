#include <string>
#include <iostream>
#include <sstream>

#include <assert.h>

#include "parser.h"
#include "exceptions.h"


/** Definitions. **/ //FIXME check if this are used somewhere. Remove o.w.

/* @TEST: try to match the @production, and return back
    to original position if not. For lookahead purpose.
*/
#define TEST(F) saveLocation();             \
                if(static_cast<bool>(F())){ \
                    removeLocation();       \
                }else{                      \
                    loadLocation();         \
                }


/* @TEST: try to match the @production, and return back
    to original position if not. For lookahead purpose.
    place in @b the returned value of @F.
*/
#define TESTB(F,b) saveLocation();               \
                   b = static_cast<bool>(F());   \
                   if(b){                        \
                       removeLocation();         \
                   }else{                        \
                       loadLocation();           \
                   }

/* Pure look ahead. */
#define CHECKB(F,b) saveLocation();                \
                    if(F()){ b=true;}else{b=false;} \
                    loadLocation();



/** Overloading for printing ASTs resulting 
    from our parsing.
**/

std::ostream& operator<< (std::ostream& out, AST const& ast){
    cout << "(" << parser::symTable[ast.tkn] << ", " << ast.lxm << ", <" 
         << ast.l << "," << ast.c << ">" << ", [";

    for(size_t i = 0; i+1 < (ast.list).size(); i++){
        AST *a = (ast.list)[i]; 
        out << *a << ",";
    }

    if(ast.list.size()>0){
        AST *a = ast.list[(ast.list).size()-1];
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
   @throws: SyntaxError() if @s is not matched.
   @return: 1 if s was matched.
*/
int
Parser::expect(Token s, string str){
    if (accept(s))
        return 1;
    string msg = string("Unexpected word: '") + lexemes[pos] 
              + string("'.\n") + str;

    throw(new SyntaxError(msg, lines[pos], columns[pos]));
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

        bool b = false;

        TESTB(rModule,b) // try to parse module

        if(!b){
            TESTB(rProperty,b)  // try to parse property
        }
        if(!b){
            // Could not match the grammar. Show where we got stuck.
            throw (new SyntaxError(string("Syntax error: '"
                                         + lexemes[pos] + "'\n")
                                  ,getLineNum(pos)
                                  ,getColumnNum(pos)));
        }
    }
    
    return 1;
}


/* @Rule: MODULE */
int
Parser::rModule(){

    if(accept(KMOD)){                   // Should be
        // This node is a MODULE node
        newNode(_MODULE, string(""));   /* FIXME can give line and col 
                                          from lastpos.
                                        */
        saveNode(_KEYWORD);
        // Get the modules name
        expect(NAME);                           // Has to be
        saveNode(_NAME);
        
        /* FIXME: Not using lookahead here :S will work as soon as
           the distinguishable word is the first one in the productions.
           Change to using TESTB otherwise.
        */
        while(rClkSec() || rVarSec() || rTranSec() || rLblSec()){;}

        saveNode();
        return 1;
    }
    return 0;
}


/* @Rule: MODULES LABELS SECTION*/
int
Parser::rLblSec(){

    if(accept(KLBL)){
        newNode(_LBLSEC,string(""));
        saveNode(_KEYWORD);
        while(rLblDef()){;}
        saveNode(); // _LBLSEC
        return 1;
    }
    return 0;
}


/* @Rule: LABEL DEFINITION */
int
Parser::rLblDef(){

    if(accept(NAME)){
        newNode(_LBL);
        if(accept(CLN)){
            saveNode(_SEPARATOR);
            try{
                expect(LDIR);
            }catch(SyntaxError *e){
                cout << e->what() << endl;
                throw string("Only 'input' and 'output' " \
                             "are accepted as labels directions.");
            }
            saveNode(_IDENT);
            try{
                expect(SCLN);
            }catch(SyntaxError *e){
                cout << e->what() << endl;
                throw string("Expected ; to end label declaration.");
            }
            saveNode(_SEPARATOR);
            saveNode(); // _LBL
            return 1;
        }
    }
    return 0;
}


/* @Rule: MODULES CLOCKS SECTION */
int
Parser::rClkSec(){
    if(accept(KCS)){
        newNode(_CLOCKSEC,string(""));
        saveNode(_KEYWORD);
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
                try{
                    expect(SCLN);
                }catch(SyntaxError *e){
                    removeNode(); //_CLOCK
                    cout << e->what() << endl;
                    throw string( "Expected semicolon to end "\
                                  "clock definition.\n");
                }
                saveNode(_SEPARATOR);
                saveNode(); //_CLOCK
                return 1;
            }else{
                removeNode(); //_CLOCK
                throw string( "Expected clock distribution. Got '"
                            + lexemes[pos] + "' instead.\nAt line "
                            + to_string(lines[pos]) + " and column " 
                            + to_string(columns[pos]));
            }
        }
        removeNode(); //_CLOCK
    }
    return 0;
}


/* @Rule: DISTRIBUTION */
int
Parser::rDistr(){

    return (rNormDist() || rExpDist() || rUniDist());
}


/* @RULE: normal distribution */
int
Parser::rNormDist(){

    if(accept(KNDIST)){
        newNode(_DISTRIBUTION, string(""));
        saveNode(_NAME);
        try{
            expect(OP);
            saveNode(_SEPARATOR);
            expect(NUM);
            saveNode(_NUM);
            expect(CMM);
            saveNode(_SEPARATOR);
            expect(NUM);
            saveNode(_NUM);
            expect(CP);
            saveNode(_SEPARATOR);
            saveNode(); // _DISTRIBUTION
            return 1;
        }catch(SyntaxError *e){
            removeNode(); // _DISTRIBUTION
            cout << e->what() << endl;
            throw string( "Normal distributions are expected to have the "
                          "following syntax: 'Normal(<NUMBER>,<NUMBER>)\n" );
        }
    }
    return 0;
}



/* @RULE: exponential distribution */
int
Parser::rExpDist(){

    if(accept(KEDIST)){
        newNode(_DISTRIBUTION, string(""));
        saveNode(_NAME);
        try{
            expect(OP);
            saveNode(_SEPARATOR);
            expect(NUM);
            saveNode(_NUM);
            expect(CP);
            saveNode(_SEPARATOR);
            saveNode(); // _DISTRIBUTION
            return 1;
        }catch(SyntaxError *e){
            removeNode(); // _DISTRIBUTION
            cout << e->what() << endl;
            throw string( "Exponential distributions are expected to have "
                          "the following syntax: 'Exponential(<NUMBER>)\n");
        }
    }
    return 0;


}


/* @RULE: uniform distribution */
int
Parser::rUniDist(){

    if(accept(KUDIST)){
        newNode(_DISTRIBUTION, string(""));
        saveNode(_NAME);
        try{
            expect(OP);
            saveNode(_SEPARATOR);
            expect(NUM);
            saveNode(_NUM);
            expect(CMM);
            saveNode(_SEPARATOR);
            expect(NUM);
            saveNode(_NUM);
            expect(CP);
            saveNode(_SEPARATOR);
            saveNode(); // _DISTRIBUTION
            return 1;
        }catch(SyntaxError *e){
            removeNode(); // _DISTRIBUTION
            cout << e->what() << endl;
            throw string( "Uniform distributions are expected to have the "
                          "following syntax: 'Uniform(<NUMBER>,<NUMBER>)\n"
                        );
        }
    }
    return 0;
}

/* @Rule: MODULES VARIABLES SECTION. */
int
Parser::rVarSec(){
    if(accept(KVS)){
        newNode(_VARSEC);
        saveNode(_KEYWORD);
        while(rVarDef()){;}
        saveNode(); // _VARSEC
        return 1;
    }
    return 0;
}

/* @Rule: INITIALIZATION. */
int
Parser::rInit(){

    if(accept(ASSIG)){
        saveNode(_SEPARATOR);
        expect(NUM);
        saveNode(_NUM);
        return 1;
    }
    return 0;
}

/* FIXME: The following method gives an example of how to use TEST and
          how to correctly use expect ... apply it to other methods.
*/
/* @Rule: VARIABLE DEFINITION. */
int
Parser::rVarDef(){

    if(accept(KVTYPE)){
        newNode(_VARIABLE, "");
        saveNode(_TYPE);
        try{
            expect(NAME, "Missing name at variable definition?\n");
            saveNode(_NAME);
            TEST(rRange);
            TEST(rInit);
            expect(SCLN, "Missing semicolon at end "
                         "of variable definition?\n");
            saveNode(_SEPARATOR);
            saveNode(); //_VARDEF
            return 1;
        }catch(SyntaxError *e){
            removeNode(); // _VARDEF
            cout << e->what() << endl;
            delete e; // FIXME do this with every other cought exception 
            throw string( "Wrong variable definition? "
                          "Expected variable definition syntax is: "
                          "'<TYPE> <NAME> ([<VAL>..<VAL>])? (= <VAL>)? ;'"
                          ".\n");
        }catch(string s){
            removeNode(); // _VARDEF
            throw s;
        }
    }
    return 0;
}


/* @Rule: RANGE */
int
Parser::rRange(){

    if(accept(OBT)){
        newNode(_RANGE, "");
        if(accept(NUM)){
            saveNode(_NUM);
        }
        if(accept(RNG)){
            saveNode(_SEPARATOR);
            try{
                expect(NUM);
                saveNode(_NUM);
                expect(CBT);
                saveNode(_SEPARATOR);
            }catch(SyntaxError *e){
                removeNode(); // _RANGE
                cout << e->what() << endl;
                throw string( "Bad range.");
            }
            saveNode(); // _RANGE
            return 1;
        }

        removeNode(); // _RANGE
    }
    return 0;

}


/* @Rule: MODULES TRANSITIONS SECTION. */
int
Parser::rTranSec(){
    if(accept(KTS)){
        newNode(_TRANSEC);
        saveNode(_KEYWORD);
        bool b = true;
        while(b){ TESTB(rTransDef,b); }
        saveNode(); // _TRANSEC
        return 1;
    }
    return 0;
}

/*
FIXME TEST and TESTB are not enough. We need to treat the locations
      also when an exceptions is risen, unless all exceptions are lethal.
*/

/* @Rule: TRANSITION. */
int
Parser::rTransDef(){

    if(accept(OBT)){
        newNode(_TRANSITION);
        saveNode(_SEPARATOR);
        if(accept(NAME)){
            saveNode(_ACTION);
            if(accept(EMARK)||accept(QMARK)){
                saveNode(_IO);
            }
        }
        expect(CBT, "Forgot ']' at transition declaration?\n");        
        newNode(_PRECONDITION,"");

        bool b = false;
        TESTB(rExpression,b)
        if(b){
            saveNode(); // _PRECONDITION
        }else{
            removeNode(); // _PRECONDITION
        }

        if(accept(CLN)){
            saveNode(_SEPARATOR);
            newNode(_ENABLECLOCK,"");
            if(accept(NAME)){
                saveNode(_NAME);
                saveNode(); // _ENABLECLOCK
            }else{
                removeNode(); // _ENABLECLOCK
            }
        }
        expect(ARROW,"Malformed precondition formula?, or forgot arrow "
                     "at transition declaration?\n");
        saveNode(_SEPARATOR);
        newNode(_POSTCONDITION);
        if(rAssigList()){
            saveNode(); //_POSTCONDITION
        }else{
            removeNode(); //_POSTCONDITION
        }
        if(accept(CLN)){
            saveNode(_SEPARATOR);
            rClkList();
        }
        expect(SCLN, "Forgot semicolon to end transition definition?\n");
        saveNode(); // _TRANSITION
        return 1;

    }
    return 0;
}


/**/
int
Parser::rAssigList(){

    newNode(_ASSIGL);
    if(rAssig()){
        while(accept(CMM)){
            saveNode(_SEPARATOR);
            if(!rAssig()){
                string msg("Malformed assignment list.\n"
                           "Unexpected " + lexemes[pos]);
                throw new SyntaxError(msg, lines[pos], columns[pos]);
            }
        }
        saveNode();
        return 1;
    }
    removeNode();
    return 0;    
}

/**/
int
Parser::rAssig(){
    newNode(_ASSIG);
    if(accept(NAME)){
        saveNode(_NAME);
        if(accept(ASSIG)){
            saveNode(_SEPARATOR);
            if(rExpression()){
                saveNode();
                return 1;
            }
        }
    }
    removeNode();
    return 0;
}

/** Expression rules. **/

/**/
int
Parser::rExpression(){
    newNode(_EXPRESSION,"");
    if(rComparison()){
        if(accept(BOP) || accept(BINOP)){
            saveNode(_OPERATOR);
            if(!rExpression()){
                string msg("Unexpected word '"+lexemes[pos]+"'.\n");
                throw new SyntaxError(msg,lines[pos], columns[pos]);
            }
        }
        saveNode(); //_EXPRESSION
        return 1;
    }
    removeNode(); //_EXPRESSION
    return 0;
}

/**/
int
Parser::rComparison(){
    newNode(_COMPARISON,"");
    if(rSum()){
        if(accept(COP)){
            saveNode(_OPERATOR);
            if(!rComparison()){
                string msg("Unexpected word '"+lexemes[pos]+"'.\n");
                throw new SyntaxError(msg,lines[pos], columns[pos]);
            }
        }
        saveNode(); //_COMPARISON
        return 1;
    }
    removeNode(); //_COMPARISON
    return 0;
}

/**/
int
Parser::rSum(){
    newNode(_SUM,"");
    if(rDiv()){
        if(accept(SUMOP)){
            saveNode(_OPERATOR);
            if(!rSum()){
                string msg("Unexpected word '"+lexemes[pos]+"'.\n");
                throw new SyntaxError(msg,lines[pos], columns[pos]);
            }
        }
        saveNode(); //_SUM
        return 1;
    }
    removeNode(); //_SUM
    return 0;
}

/**/
int
Parser::rDiv(){
    newNode(_DIV,"");
    if(rValue()){
        if(accept(DIVOP)){
            saveNode(_OPERATOR);
            if(!rDiv()){
                string msg("Unexpected word '"+lexemes[pos]+"'.\n");
                throw new SyntaxError(msg,lines[pos], columns[pos]);
            }
        }
        saveNode(); //_DIV
        return 1;
    }
    removeNode(); //_DIV
    return 0;
}

/**/
int
Parser::rValue(){
    
    newNode(_VALUE,"");
    if(accept(NUM)){
        saveNode(_NUM);
    }else if(accept(NAME)){
        saveNode(_NAME);
    }else if(accept(BOOLV)){
        saveNode(_BOOLEAN);
    }else if(accept(OP)){
        saveNode(_SEPARATOR);
        if(!rExpression()){
            string msg("Unexpected word '"+lexemes[pos]+"'.\n");
            throw new SyntaxError(msg,lines[pos], columns[pos]);            
        }
        expect(CP,"Missing ')'?\n");
        saveNode(_SEPARATOR);
    }else if(accept(EMARK)){
        if(accept(EMARK)){
            saveNode(_NEGATION);
            if(!rExpression()){
                string msg("Unexpected word '"+lexemes[pos]+"'.\n");
                throw new SyntaxError( msg,lines[pos]
                                     , columns[pos]);            
            }
        }
    }//else if (accept(-)) ... FIXME
     else{
        removeNode(); //_VALUE
        return 0;
    }
    saveNode(); //_VALUE
    return 1;
}

/** End Expression rules. **/

/**/
int
Parser::rClkList(){
    newNode(_RESETCLOCKLIST); //FIXME should be here or outside?
    if(accept(NAME)){
        saveNode(_RESETCLOCK);
        while(accept(CMM)){
            saveNode(_SEPARATOR);
            expect(NAME, "Missing clock or spare semicolon.\n");
        }
        saveNode();
        return 1;
    }
    removeNode();
    return 0;
}


/** Property rules. **/

int
Parser::rProperty(){

    if(accept(KPROP)){
        newNode(_PROPERTY,"");
        saveNode(_KEYWORD);
        expect(NAME, "Missing property name?\n"); //FIXME maybe no necessary
        saveNode(_NAME);
        expect(CLN, "Missing colon after property name?\n");
        saveNode(_SEPARATOR);
        expect(SCLN, "Missin semicolon to end property declaration?\n");
        saveNode(_SEPARATOR);
        saveNode(); //_PROPERTY
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
    }catch(string s){
        cout << "Parser: " << s << endl;
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


}// namespace Parser
