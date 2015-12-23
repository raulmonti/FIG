//==============================================================================
//
//    Parser module for FIG
//    Raul Monti
//    2015
//
//==============================================================================


#ifndef PARSER_H
#define PARSER_H

#include <stack>
#include <vector>
#include <string>
#include <FlexLexer.h>
#include "config.h"
#include "ast.h"

using namespace std;

namespace parser{

/* The token representation for each lexeme. 
*/
typedef enum    { MEOF    // my end of file symbol
                , NUM     // float type numbers
                , KPROP   // Keyword PROPERTY
                , KMOD    // keyword "module"
                , KEMOD   // keyword "end module"
                , KCLOCK  // keyword clock
                , KNDIST  // keyword Normal
                , KEDIST  // keyword Exponential
                , KUDIST  // keyword Uniform
                , KINIT   // Keyword "init"
                , ITYPE   // keyword Int
                , BTYPE   // Keyword Bool
                , KCONST  // keyword const
                , NAME    // strings starting with letters
                , XNAME   // strings starting with letters and ended by '.
                , WS      // white spaces (' \t')
                , NL      // new line ('\n')
                , INT     // integer
                , OB      // {
                , CB      // }
                , OBT     // [
                , CBT     // ]
                , OP      // (
                , CP      // )
                , SCLN    // ;
                , CLN     // :
                , CMM     // ,
                , PLUS    // +
                , MINUS   // - 
                , DIVOP   // * / %
                , COP     // <= >= < >
                , BOP     // == !=
                , BOOLV   // true false
                , AMP     // &
                , MID     // |
                , ASSIG   // =
                , DOT     // .
                , ARROW   // ->
                , RNG     // range ..
                , EMARK   // !
                , QMARK   // ?
                , COMMENT // C style comment (/* ... */)
                , AT      // @
                , AP      // '
                , DUM     // dummy symbol 
                } Token;


/* prodSym is used to identify each production of the 
   grammar. The _ is used to easily distinguish them 
   from the Token type ones. This enumeration is used
   to fill in the @s member of the AST class.
*/
typedef enum{ _EOF            // End of File
            , _DUMMY
            , _MODEL
            , _MODULE
            , _CONST
            , _VARIABLE
            , _TRANSITION       // Transition
            , _CLOCK
            , _SETC
            , _KEYWORD
            , _NAME
            , _INT
            , _NUM
            , _SEPARATOR     // ; : , { } [ ] . ...
            , _DISTRIBUTION
            , _IDENT
            , _TYPE
            , _RANGE
            , _INIT         // initialization of variables
            , _ACTION
            , _IO           // input or output as ? !
            , _ENABLECLOCK  // output transition enable clock
            , _PRECONDITION
            , _POSTCONDITION
            , _RESETCLOCKLIST
            , _RESETCLOCK
            , _EXPRESSION   // a {&&,||} b
            , _EQUALITY        // a {==,!=} b
            , _COMPARISON   // a {<,>,<=,>=} b
            , _SUM
            , _DIV
            , _VALUE
            , _OPERATOR
            , _NEGATION     // !
            , _ASSIGL
            , _ASSIG
            , _BOOLEAN     // true false
            , _PROPERTY
            , _MINUS       // -
            } prodSym;



/* The next table is the human readable version of prodSym. 
   For any enumerate e in prodSym, symTable[e] should return 
   the printable representation of e.
*/
static const char symTable[][25] =
    {"EOF","DUMMY","MODEL","MODULE", "CONSTANT", "VARIABLE",
     "TRANSITION","CLOCK", "SET CLOCK", "KEYWORD","NAME","INT","REAL",
     "SEPARATOR", "DISTRIBUTION",
     "IDENTIFIER", "TYPE","RANGE", "INITIALIZATION" ,"ACTION",
     "INPUT/OUTPUT",
     "ENABLING CLOCK", "PRECONDITION FORMULA", "POSTCONDITION ASSIGNMENT",
     "CLOCK RESETS LIST", "CLOCK TO RESET", 
     "EXPRESSION", "EQUALITY", "COMPARISON", "SUMMATION",
     "DIVITION", "VALUE", "BOOLEAN/MATH OPERATOR",
     "NEGATION", "ASSIGNMENT LIST", "ASSIGNMENT", "TRUE OR FALSE VALUE",
     "VERIFICATION PROPERTY", "MINUS"
    };




// Parser Class ================================================================

class Parser
{ 
    FlexLexer* lexer;               // The Lexer

    /* When lexing a string, each lexem will be kept in
       @lexemes, and its representing token will
       be kept in @tokens at the same position.
    */
    vector<Token>  tokens;          // Vector with tokens.
    vector<string> lexemes;          // Vector with lexemes.
    vector<int>    lines;
    vector<int>    columns;

    /* */
    stack<int>   lastk;             // Stack for saving locations to lookahead.
    Token        tkn;               // Current extracted token.
    int          pos;               // Actual position of the parser in tokens.
    int          lastpos;           // Position of the last accepted lexeme.
    stack<Node*> astStk;            // Stack for bulding up the AST
    bool         skipws;            // Skip white spaces?.



public:

    /** 
     */
    Parser(void);

    /**
     */
    virtual ~Parser();

    /**
     *  @brief Parse from stream @str, and build the resulting AST into
     *  the parameter @result.
     *  @return 1 if successful, 0 otherwise.
     */
    int
    parse(stringstream *str, AST * & result);

    
    /**  @brief check if we ran out of lexemes to parse.
     *   @return 1 if parsing ended, 0 otherwise.
     */
    inline int
    ended(){
        return pos == tokens.size()-1;
    }


private:


    /** Building the AST **/

    /** @brief prepare a new node for abstract syntax tree.
     *  Should be called at the beginning of matching of grammar production,
     *  enabling to save the result of parsing afterwards using @saveNode
     *  method.
     *  @param str The parsed string for the node.
     *  @param tkn The grammar type for this parsed string.
     *  @param line Line number. 
     *  @param col Column number.
     */
    int
    newNode(prodSym tkn, string str, int line = 0, int col = 0);


    /** @param newNode When called only with parameter @tkn, it is
     *  a shortcut for building a new node with the last accepted
     *  lexeme.
     *  @param tkn The grammar type for this parsed string.
     */
    int
    newNode(prodSym tkn);


    /* @saveNode: Should be called after a correct match of a grammar
       saving the production into the AST.
       @line, @col: line and column number respectively.
    */
    int
    saveNode();

    /* @saveNode: when called only with parameters @tkn
       it is a shortcut to to saving a new node with the last accepted
       lexeme.
    */
    int
    saveNode(prodSym tkn);

    /* @saveNode: when called with parameters @str, @tkn, @line, and @col
       it is a shortcut to <newNode(str,tkn,line,col); saveNode();>
    */
    int
    saveNode(prodSym tkn, string str, int line = 0, int col = 0);


    /* @removeNode: should be called if the current production was not
       successfully matched.
    */
    int
    removeNode();


    /** Parsing methods **/

    /* @Ask if the token in @tkn can be considered a white space
        symbol.
       @return: 1 if yes, 0 if not.
    */
    inline int 
    isw(){
        return (tkn == WS || tkn == NL);
    }


    /* @nextLxm: make next lexed token available in @tkn class member.
        If @skipws then will skip white space tokens and make available
        the next non white one. If it runs out of tokens the @tkn value
        will be MEOF.
    */
    void nextLxm(void);


    /* @Consume next token and check if it matches with @s.
       @return: 1 if it matches, 0 if it does not.
    */
    int accept(Token s);
 

    /* @Consume the next token and trow a SyntaxError exception if it 
        does not match with @s.
       @s: the expected symbol.
       @str: error string explaining the problem with not matching @s.
       @ throw: SyntaxError if next token does not match @s.
       @return: 1 if s was matched. Throw an exception otherwise.
    */
    int expect(Token s, string str = "");


    /** Grammar look ahead. **/

    /* @saveLocation: save state to be able to backtrack if a grammar
       is not finally matched.
    */
    inline int
    saveLocation(){
        lastk.push(pos);
    }


    /* @loadLocation: When grammar did not match, return to the saved state.
    */
    int 
    loadLocation();


    /* @removeLocation: when we do not need to keep the last 
       saved location, normaly because the grammar involved
       was matched, we should remove it from our saved locations.
    */
    inline int
    removeLocation(){
        lastk.pop();
    }


    /** Grammar Rules **/


    /* @The starting point of the grammar to be parsed with the
        recursive descent parser.
       @return: ...
    */
    int 
    rGrammar();

    /* @Rule: Module */
    int
    rModule();

    /**
     * @brief rule for constant declarations.
     */
    int
    rConstant();

    /* @Rule:  Clock seting */
    int
    rSetClock();

    /* @Rule: CLOCK DEFINITION */
    int
    rClkDef();

    /* @Rule: Distribution */
    int
    rDistr();

    /* @RULE: normal distribution */
    int
    rNormDist();

    /* @RULE: exponential distribution */
    int
    rExpDist();

    /* @RULE: uniform distribution */
    int
    rUniDist();

    /* @Rule: VARIABLE DEFINITION. */
    int
    rVarDef();

    /* @Rule: TRANSITION. */
    int
    rTransDef();

    /**/
    int
    rExpression();

    /**/
    int
    rEqual();

    /**/
    int
    rComparison();

    /**/
    int
    rSum();

    /**/
    int
    rDiv();

    /**/
    int
    rValue();

    /* @Rule: ASSIGNMENT. */
    int
    rAssig();

    /**/
    int
    rProperty();

}; // End class Parser.



} // End namespace parser.


#endif //PARSER_H