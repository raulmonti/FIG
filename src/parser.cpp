//==============================================================================
//
//    Parser module for FIG
//    Raul Monti
//    2015
//
//==============================================================================


#include <string>
#include <iostream>
#include <sstream>
#include <assert.h>
#include "parser.h"
#include "exceptions.h"


//==============================================================================

/** @brief Overloading for printing ASTs resulting from our parsing.
 */

std::ostream& operator<< (std::ostream& out, AST const& ast){
    cout << "(" << parser::symTable[ast.tkn] << ", " << ast.lxm << ", <" 
         << ast.l << "," << ast.c << ">" << ", [";

    for(size_t i = 0; i+1 < (ast.branches).size(); i++){
        AST *a = (ast.branches)[i]; 
        out << *a << ",";
    }

    if(ast.branches.size()>0){
        AST *a = ast.branches[(ast.branches).size()-1];
        out << *a;
    }
    cout << "])";
    return out;
}


using namespace std;




// Parser class implementation =================================================

namespace parser{


/** @brief Parser class constructor.
 */ 
Parser::Parser(void){
    lexer       = new yyFlexLexer;
    pos         = -1;
    lastpos     = pos;
    skipws      = true;
    ast         = NULL;
}


/** @brief Parser class destructor.
 */ 
Parser::~Parser(){
    delete lexer;
}


/** @brief Get the next token from the tokens vector and make it
 *  available in @tkn class member. If @skipws then it will skip
 *  white space tokens and make available the next non white 
 *  one. If the end of the tokens vector is reached the @tkn
 *  will contain the MEOF token.
 */
void
Parser::nextLxm(void){

    assert(pos < static_cast<int>(tokens.size()));
    assert(pos < tokens.size()-1 || (Token)tokens[pos] == MEOF);

    pos++;
    tkn = (Token)tokens[pos];

    // Skip whites and comments if told too.
    if( (skipws && isw()) || tokens[pos] == COMMENT ){
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
        nextLxm();
        return 1;
    }
    return 0;    
}
 
/**
 * @Brief Consume the next token and trow an exception if it 
 * does not match with @s.
 * @param [in] s The expected token.
 * @throw SyntaxError() if @s is not matched.
 * @return 1 if s was matched.
 */
int
Parser::expect(Token s, string str){
    if (accept(s))
        return 1;
    string msg = string("Unexpected word: '") + lexemes[pos] 
              + string("'.\n") + str;

    throw(new SyntaxError(msg, lines[pos], columns[pos]));
}


//==============================================================================


/** 
 * @Brief When grammar did not match, return to the saved state.
 * @Note For looking ahead in the grammar.
 */
void 
Parser::loadLocation(){
    pos = lastk.top();
    lastk.pop();
    tkn = (Token)tokens[pos];
}




//==============================================================================
// Grammar Rules
//==============================================================================


/**
 * @brief  The starting point of the grammar to be parsed with the
 *         recursive descent parser.
 * @return 1 if successfully parsed. Throw SyntaxError otherwise.
 */
int
Parser::rGrammar(){

    newNode(_MODEL, string(""));
    while(!ended()){
        // try to parse module
        if(!rModule()){
            // try to parse a global constant
            if(!rConstant()){
                // Could not match the grammar. Show where we got stuck.
                throw (new SyntaxError( "Syntax error: '" + lexemes[pos] + "'\n"
                                      , lines[pos]
                                      , columns[pos]));
            }
        }
    }

    return 1;
}



//==============================================================================


/**
 * @brief Rule CONSTANT.
 */
int
Parser::rConstant(){
    
    if(accept(KCONST)){
        newNode(_CONST, "", lines[lastpos],columns[lastpos]);
        saveNode(_KEYWORD);
        if(!accept(ITYPE)){
            expect(BTYPE, "Missing type for constant declaration.\n");
        }
        saveNode(_TYPE);
        expect(NAME, "Missing name for constant declaration.\n");
        saveNode(_NAME);
        expect(ASSIG, "Missing '='.\n");
        saveNode(_SEPARATOR);
        if(!rExpression()){
            // FIXME this should throw a malformed expression exception.            
            return 0;
        }
        expect(SCLN, "Missing ';'.\n");
        saveNode(); // _CONST
        return 1;
    }

    return 0;
}


//==============================================================================


/**
 * @brief Rule MODULE.
 */
int
Parser::rModule(){

    if(accept(KMOD)){
        // This node is a MODULE node
        newNode(_MODULE, string(""),lines[lastpos],columns[lastpos]);   
        saveNode(_KEYWORD);
        // Get the modules name
        expect(NAME);                   // Has to be
        saveNode(_NAME);
        // body
        while(rVarDef() || rClkDef() || rTransDef() ){;}
        // close the module
        expect(KEMOD, "Did you forget to close a module?.\n");
        saveNode(_SEPARATOR);
        saveNode(); // _MODULE
        return 1;
    }
    return 0;
}


//==============================================================================


/**
 * @brief Rule CLOCK DEFINITION.
 */
 
int
Parser::rClkDef(){
    saveLocation();
    if (accept(NAME)){
        newNode(_CLOCK, "",lines[lastpos],columns[lastpos]);
        saveNode(_NAME);
        expect(CLN, "Missing ':'.\n");
        saveNode(_SEPARATOR);
        if(accept(KCLOCK)){
            saveNode(_TYPE);
            expect(SCLN, "Missing ';'.\n");
            saveNode(_SEPARATOR);
            saveNode(); // _CLOCK
            removeLocation();
            return 1;
        }
        removeNode(); //_CLOCK

    }
    loadLocation();
    return 0;
}


//==============================================================================

//TODO Depict here how variables are being parsed. Do the same with the
//     rest of the rulles. 
/** 
 * @Brief Rule VARIABLE DEFINITION.
 */
int
Parser::rVarDef(){
    saveLocation();
    if(accept(NAME)){
        newNode(_VARIABLE, "",lines[lastpos],columns[lastpos]);
        saveNode(_NAME);
        expect(CLN, "Missing ':'.\n");
        if(accept(BTYPE)){
            saveNode(_TYPE);
        }else if(accept(OBT)){
            newNode(_RANGE, "");
            saveNode(_SEPARATOR);
            expect(NUM, "Bad range. Forgot lower limit?\n");
            saveNode(_NUM);
            expect(RNG, "Bad range. Forgot .. ?\n");
            saveNode(_SEPARATOR);
            expect(NUM, "Bad range. Forgot upper limit?\n");
            saveNode(_NUM);
            expect(CBT, "Bad range. Forgot ] ?");
            saveNode(_SEPARATOR);
            saveNode(); // _RANGE
        }else{
            loadLocation();
            removeNode(); // _VARIABLE
            return 0;
        }
        // Optional initialization
        if(accept(KINIT)){
            newNode(_INIT);
            saveNode(_KEYWORD);
            //rExpression(); TODO
            expect(NUM, "Missing initial value.");
            saveNode(_NUM);
            saveNode(); // _INIT
        }
        expect(SCLN,"Missing ';'.\n");
        saveNode(); // _VARIABLE
        removeLocation();
        return 1;
    }
    loadLocation();
    return 0;
}


//==============================================================================


/**
 * @Brief Rule TRANSITION. 
 */
int
Parser::rTransDef(){

    if(accept(OBT)){
        newNode(_TRANSITION,"");
        saveNode(_SEPARATOR);
        if(accept(NAME)){   
            saveNode(_ACTION);
            if(accept(EMARK) || accept(QMARK)){
                saveNode(_IO);
            }else{
                // No mark transitions are output.
                saveNode(_IO,"!");
            }
        }else{
            // silent transition.
            saveNode(_ACTION,"");
            saveNode(_IO,"!");
        }
        expect(CBT, "Forgot ']' at transition declaration?\n");    
        saveNode(_SEPARATOR);    
        newNode(_PRECONDITION,"");
        if(rExpression()){
            saveNode(); // _PRECONDITION
        }else{
            removeNode(); // _PRECONDITION
        }

        newNode(_ENABLECLOCK,"");
        if(accept(AT)){
            saveNode(_SEPARATOR);
            expect(NAME, "Forgot a clock name?\n");
            saveNode(_NAME);
            saveNode(); // _ENABLECLOCK
        }else{
            removeNode();   // _ENABLECLOCK
        }

        expect(ARROW,"Malformed precondition formula?, or forgot arrow "
                     "at transition declaration?\n");
        saveNode(_SEPARATOR);
        newNode(_POSTCONDITION);

        if(rAssig() || rSetClock()){
            while(accept(AMP)){
                saveNode(_SEPARATOR);
                if(!(rAssig() || rSetClock())){
                    return 0;
                }
            }
        }
        
        saveNode(); //_POSTCONDITION
        expect(SCLN, "Forgot semicolon to end transition definition?\n");
        saveNode(); // _TRANSITION
        return 1;

    }
    return 0;
}


//==============================================================================

/**
 */
int
Parser::rAssig(){
    saveLocation();
    if(accept(OP)){
        newNode(_ASSIG);
        saveNode(_SEPARATOR);
        expect(XNAME, "Missing <next name>.\n");
        saveNode(_NAME);
        expect(ASSIG, "Missing '='.\n");
        saveNode(_SEPARATOR);
        if(rExpression()){
            expect(CP, "Missing ')'.\n");
            saveNode(_SEPARATOR);
            saveNode(); // _ASSIG
            removeLocation();
            return 1;
        }
        removeNode(); // _ASSIG
    }
    loadLocation();
    return 0;
}


//==============================================================================

/**
 */
int
Parser::rSetClock(){
    saveLocation();
    if(accept(OP)){
        newNode(_SETC);
        saveNode(_SEPARATOR);
        expect(XNAME, "Missing <next name>.\n");
        saveNode(_NAME);
        expect(ASSIG, "Missing '='.\n");
        saveNode(_SEPARATOR);
        if(rDistr()){
            expect(CP, "Missing ')'.\n");
            saveNode(_SEPARATOR);
            saveNode(); // _ASSIG
            removeLocation();
            return 1;
        }
        removeNode(); // _ASSIG
    }
    loadLocation();
    return 0;
}


//==============================================================================


/**
 * @Breif Rule DISTRIBUTION 
 */
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


/** Expression rules. **/

/**/
int
Parser::rExpression(){
    newNode(_EXPRESSION,"",lines[pos],columns[pos]);
    if(rEqual()){
        if(accept(AMP) || accept(MID)){
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
Parser::rEqual(){
    newNode(_EQUALITY,"",lines[pos],columns[pos]);
    if(rComparison()){
        if(accept(BOP)){
            saveNode(_OPERATOR);
            if(!rEqual()){
                string msg("Unexpected word '"+lexemes[pos]+"'.\n");
                throw new SyntaxError(msg,lines[pos], columns[pos]);
            }
        }
        saveNode(); //_EQUALITY
        return 1;
    }
    removeNode(); //_EQUALITY
    return 0;
}


/**/
int
Parser::rComparison(){
    newNode(_COMPARISON,"",lines[pos],columns[pos]);
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
    newNode(_SUM,"",lines[pos],columns[pos]);
    if(rDiv()){
        if(accept(MINUS)||accept(PLUS)){
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
    newNode(_DIV,"",lines[pos],columns[pos]);
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
    
    newNode(_VALUE,"",lines[pos],columns[pos]);
    if(accept(NUM)){
        saveNode(_NUM);
    }else if(accept(NAME)){ // don't know the type
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
        saveNode(_NEGATION);
        if(!rExpression()){
            string msg("Unexpected word '"+lexemes[pos]+"'.\n");
            throw new SyntaxError( msg,lines[pos]
                                 , columns[pos]);            
        }
    }else if(accept(MINUS)){
        saveNode(_MINUS);
        if(!rValue()){
            string msg("Unexpected word '"+lexemes[pos]+"'.\n");
            throw new SyntaxError( msg,lines[pos]
                                 , columns[pos]);            
        }
    }else{
        removeNode(); //_VALUE
        return 0;
    }
    saveNode(); //_VALUE
    return 1;
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
        if(!rExpression()){
            string msg("Missing expression in property declaration.\n");
            throw new SyntaxError( msg,lines[pos]
                                 , columns[pos]);
        }
        expect(SCLN, "Missing semicolon to end property declaration?\n");
        saveNode(_SEPARATOR);
        saveNode(); //_PROPERTY
        return 1;
    }
    return 0;
}   


////////////////////////////////////////////////////////////////////////////////

/**
 * @brief  Parse stream @str and place the resulting AST in @result.
 *         It is caller responsibility to free the allocated AST 
 *         @result afterwards.
 * @return 0 if something went wrong (print exception), 1 otherwise.
 */
const pair< AST*, parsingContext> 
Parser::parse(stringstream *str){

    // FIXME lineno is already given by the lexer (yylineno)
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
                }else if(ret == COMMENT){ //manually count
                    string comment = lexer->YYText();
                    for(int i = 0; i < lexer->YYLeng(); i++){
                        if(comment[i]=='\n'){
                            lineno++;
                            colnum = 1;
                        }else{
                            colnum++;
                        }
                    }
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
            ast = astStk.top();
        }

        //
        fill_context();
        //

    }catch(exception *e){
        cout << "[Parser ERROR] " << e->what() << endl;
        delete e;
    }catch(string s){
        cout << "[Parser ERROR] " << s << endl;
    }
    return make_pair( ast, mPc );
}


//==============================================================================


/**
 * @brief Fill the context @mPc for @ast. Check for variables declarations
 *        to be correct.
 */
int
Parser::fill_context(){

    assert(ast && "Now parsed AST found.");

    string error_list = "";
    vector<AST*> constants = ast->get_all_ast(_CONST);
    vector<AST*> modules = ast->get_all_ast(_MODULE);
    
    // Fill map with constants.
    for(int i = 0; i < constants.size(); ++i){
        string name = constants[i]->get_lexeme(_NAME);
        Type t = str2Type(constants[i]->get_lexeme(_TYPE));
        mPc.insert( pvtm(name,ptm(t,"")));
    }

    for(int i = 0; i < modules.size(); i++){
        vector<AST*> variables = modules[i]->get_all_ast(_VARIABLE);
        vector<AST*> clocks = modules[i]->get_all_ast(_CLOCK);
        string module = modules[i]->get_lexeme(_NAME);
        // Fill map with variables.
        for(int j=0; j < variables.size(); j++){
            string name = variables[j]->get_lexeme(_NAME);
            string type = variables[j]->get_lexeme(_TYPE);
            if(type == "bool"){
                mPc.insert(pvtm(name,ptm(T_BOOL,module)));
                // The variable in <next state>
                mPc.insert(pvtm(name+"'",ptm(T_BOOL,module)));
            }else{
                AST* range = variables[j]->get_first(_RANGE);
                assert(range);
                vector<string> limits = range->get_list_lexemes(_NUM);
                assert(limits.size() == 2);
                if(stoi(limits[0]) > stoi(limits[1])){
                    string pos = variables[j]->get_pos();
                    error_list.append("[ERROR] Empty range in variable "
                        "declaration at " + pos + ".\n");
                }
                mPc.insert(pvtm(name,ptm(T_ARIT,module)));
                mPc.insert(pvtm(name+"'",ptm(T_ARIT,module)));
            }
        }
        // Fill map with clocks. 
        for(int j=0; j < clocks.size(); j++){
            string name = clocks[j]->get_lexeme(_NAME);
            mPc.insert(pvtm(name, ptm(T_CLOCK, module)));
        }
    }

    if(error_list != ""){
        throw error_list;
    }

    return 1;
}


//==============================================================================

/**
 * @brief Translate the parsed type string into a type in our Type enumeration.
 */
Type
Parser::str2Type(string str){
    Type result;
    if(str == "int"){
        result = T_ARIT;
    }else if(str == "bool"){
        result = T_BOOL;
    }else if(str == "clock"){
        result = T_CLOCK;
    }else{
        result = T_NOTYPE;
    }
    return result;
}



////////////////////////////////////////////////////////////////////////////////
//
// AST construction methods
//
////////////////////////////////////////////////////////////////////////////////


int
Parser::newNode(prodSym tkn,string str, int line, int col){
    
    Node *node = new Node(tkn,str,line,col);
    astStk.push(node);
}


int
Parser::newNode(prodSym tkn){
    
    Node *node = new Node( tkn,lexemes[lastpos],lines[lastpos]
                         , columns[lastpos]);
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
