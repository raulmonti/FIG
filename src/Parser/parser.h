#ifndef PARSER_H
#define PARSER_H

#include <stack>
#include <vector>
#include <string>
#include <FlexLexer.h>
#include "debug.h"


using namespace std;

namespace parser{


/* The symbols corresponding to the characterization of
   each word that is going to be lexed.
*/
typedef enum    { MEOF    // my end of file symbol
                , NUM     // float type numbers
                , KMOD    // MODULE
                , KCS     // keyword CLK
                , KVS     // keyword VAR
                , KTS     // keyword TRANS
                , NAME    // strings starting with letters
                , WS      // white spaces (' \t')
                , NL      // new line ('\n')
                , INT     // integer
                , OB      // {
                , CB      // }
                , OBT     // [
                , CBT     // ]
                , SCLN    // ;
                , CLN     // :
                , MOP     // + - * / %
                , COP     // == <= >= != < >
                , BOP     // || && !
                , ASG     // =
                , DOT     // .
                , DUM     // dummy symbol
                } Symbol;

/* prodSym is used to identify each production of the grammar. The _
   is used to easily distinguish them from the Symbol type ones.
*/
typedef enum    { _EOF, _NUM, _INT, _NM, _KMOD, _KVAR, _KCS, _KTRN, _CLK, 
                  _CLN, _SCLN, _DSTR,
                  _DUM
                } prodSym;


static const char symTable[][11] =
    {"EOF","NUM","MODULE","CLKS","VARS","TRANS","NAME","WS","NL","INT"};








/*** The Abstract Syntax Tree class ***/

class AST
{
public:

    string n;       // name
    int s;          // symbol
    int ln;         // line number
    int cl;         // column number
    vector<AST*> l; // list of children
 
    // Constructor.
    AST(void);
    AST(int symbol = _DUM, string name ="", int line = 0, int col = 0);

    // Destructor.
    virtual ~AST();

    // @Push back a new child into @l.
    inline void pb(AST *c)
    {
        l.push_back(c);
    }
};

typedef AST Node; // Node is the same as AST.









/*** Parser Class ***/

class Parser
{ 
    /* */
    FlexLexer* lexer;            // The Lexer

    /* When lexing a string, each lexed word will be kept in
       @strvec, and the symbol of the corresponding type will
       be kept in @symvec at the same position. This is to
       be used later by the parser.
    */
    vector<Symbol> symvec;  // Vector with lexed tokens.
    vector<string> strvec;  // Vector with lexed words.
    int            lineno;  
    int            colnum;

    /* */
    stack<int>   lastk;           // Stack for saving locations to lookahead.
    Symbol       sym;             // Current extracted token.
    int          pos;             // Actual position of the parser in symvec.
    int          lastpos;         // Position of the last accepted symbol.
    string       lastAcc;         // Last accepted token.
    stack<Node*> astStk;
    bool         skipws;          // Skip white spaces?.



public:

    /**/
    Parser(void);

    /**/
    virtual ~Parser();

    /* @Parse: Parse from stream @str, and build the resulting AST into
       the parameter @result.
       @return: 1 if successful, 0 otherwise.
    */
    int
    parse(stringstream *str, AST * & result);

    
    /* @Parsing ends when there is no more words to check
       at the lexed vector.
       @return: 1 if parsing ended, 0 otherwise.
    */
    inline int
    ended(){
        return pos == symvec.size()-1;
    }
private:


/*** Building the AST ***/

    /* @newNode: add a new node to the parsers abstract syntax tree stack.
       Should be called at the beginning of matching of grammar production,
       enabling to save the result of parsing.
       @str: the parsed string for the node.
       @sym: the grammar type for this parsed string.
    */
    int
    newNode(prodSym sym, string str, int line = 0, int col = 0);

    /* @saveNode: Should be called after a correct match of a grammar
       production. It attaches the top Node from the stack to its
       father, and removes it from the stack since it is a completed
       production.
    */
    int
    saveNode();

    /* @saveNode: when called with params @str and @sym, it is a shortcut
       to <newNode(str,sym); saveNode();>
    */
    int
    saveNode(prodSym sym, string str, int line = 0, int col = 0);

    /* @removeNode: should be called if the current production was not
       successfully matched.
    */
    int
    removeNode();


/*** Parsing methods ***/


    /* @Get the line number for the word at position @p in
        the lexed words.
       @p: the position of the word to ask for. Should be valid
        position, i.e. >= 0 and < number of words lexed. 
    */
    int
    getLineNum(int p);

    /* @Get the column number for the starting position of word at 
        position @p in the lexed words.
       @p: the position of the word to ask for. Should be valid
        position, i.e. >= 0 and < number of words lexed. 
    */
    int 
    getColumnNum(int p);

    /* @Ask if the symbol in @sym can be considered a white space
        symbol.
       @return: 1 if yes, 0 if not.
    */
    inline int 
    isw(){
        return (sym == WS || sym == NL);
    }

    /* @Read the next symbol from the lexed vector and make it
        available in @sym class member. If @skipws then will skip
        white space symbols and make available the next non white 
        one. If the end of the lexed symbols is reached the @sym
        will contain the MEOF symbol.
    */
    void nextSym(void);
 
    /* @Consume next symbol and check if it matches with @s.
       @return: 1 if it matches, 0 if it does not.
    */
    int accept(Symbol s);
 
    /* @Consume the next symbol and trow an exception if it 
        does not match with @s.
       @s: the expected symbol.
       @return: 1 if s whas matched.
    */
    int expect(Symbol s);


/*** For looking ahead in the grammar. ***/


    /* @Save state to be able to backtrack if a grammar
       is not finally matched.
    */
    inline int
    saveLocation(){
        lastk.push(pos);
    }

    /* @When grammar did not match, return to the saved state.
    */
    int 
    loadLocation();

    /* @When we do not need to keep the last saved location,
       mainly because the grammar involved was matched, we
       can remove it from our saved locations.
    */
    inline int
    removeLocation(){
        lastk.pop();
    }


/*** Grammar Rules ***/


    /* @The starting point of the grammar to be parsed with the
        recursive descent parser.
       @return: ...
    */
    int 
    rGrammar();

    /* @Rule: Module */
    int
    rModule();

    /* @Rule: Modules clocks section */
    int
    rClkSec();

    /* @Rule: Modules variables section */
    int
    rVarSec();

    /* @Rule: Modules transitions section */
    int
    rTraSec();

    /* @Rule: CLOCK DEFINITION */
    int
    rClkDef();

    /* @Rule: Distribution */
    int
    rDistr();

}; // End class Parser.



} // End namespace parser.


/*** '<<' overloading for the AST structure ***/

std::ostream& operator<< (std::ostream& out, parser::AST const& ast);


#endif
