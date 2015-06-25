#ifndef PARSER_H
#define PARSER_H

#include <stack>
#include <vector>
#include <string>
#include <FlexLexer.h>


using namespace std;

namespace parser{


/* The symbols corresponding to the caracterization of
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
                } Symbol;


static const char symTable[][11] =
    {"EOF","NUM","MODULE","CLKS","VARS","TRANS","NAME","WS","NL","INT"};





/* Parser Class */
class Parser
{ 
    /* */
    FlexLexer* lexer;            // The Lexer

    /* When lexing a string, each lexed word will be kept in
       @strvec, and the symbol of the corresponding tipe will
       be kept in @symvec at the same position. This is to
       be used later by the parser.
    */
    std::vector<Symbol> symvec;  // Vector with lexed symbols.
    std::vector<string> strvec;  // Vector with lexed words.

    /* */
    stack<int>  lastk;           // Stack for saving locations to lookahead.
    Symbol      sym;             // Current stracted symbol.
    int         pos;             // Actual position of the parser in symvec.
    int         lastpos;         // Position of the last accepted symbol.
    bool        skipws;          // Skip white spaces?.

public:

    /**/
    Parser(void);

    /**/
    virtual ~Parser();


private:

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
        abalable in @sym class member. If @skipws then will skip
        white space symbols and make abalable the next non white 
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

    /* @Save state to be able to bacjtrack if a grammar
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
       mainly beacause the grammar involved whas matched, we
       can remove it from our saved locations.
    */
    inline int
    removeLocation(){
        lastk.pop();
    }

/*** Grammar Rules ***/

    /* @The starting point of the grammar to be parsed with the
        recurive descent parser.
       @return: ...
    */
    int 
    grammar();

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

public:


    /* Set input stream from which to parse.
    */
    inline int
    setStream(stringstream *ss){
        lexer->switch_streams((istream *)ss);
    }


    /* @Parse ...
       @return: ...
    */
    int
    parse(stringstream *str);

    
    /* @Parsing ends when there is no more words to check
       at the lexed vector.
       @return: 1 if parsing ended, 0 otherwise.
    */
    inline int
    ended(){
        return pos == symvec.size()-1;
    }

}; // End class Parser.



} // End namespace parser.

#endif
