/**

    Abstract syntax tree module
    Raul Monti
    2015

**/


#ifndef AST_H
#define AST_H

#include <vector>
#include <string>
#include <sstream>


using namespace std;

/** The Abstract Syntax Tree class *******************************************/


typedef int Key;

class AST
{
public:

    string          lxm;        // lexeme
    int             tkn;        // token
    int             l;          // line number
    int             c;          // column number
    vector<AST*>    list;       // list of children
 
    // @Constructor.
    AST(void);
    AST(int token = 0, string lexeme = "", int line = 0, int col = 0);

    // @Destructor.
    virtual ~AST();

    // @Push back a new child into @l.
    inline void pb(AST *c)
    {
        list.push_back(c);
    }

    /* @get_list: get a vector with pointers to every child AST with token k.
    */
    vector<AST*>
    get_list(int k);

    /* @get_list_lexemes: get a vector with the lexeme of every child AST
        with token k.
    */
    vector<string>
    get_list_lexemes(int k);

    /* @get_lexeme: Get the lexeme of the first child AST with token k;
       @return: "" if not found, the lexeme as a string otherwise.
    */
    string
    get_lexeme(int k);

};

typedef AST Node; // Node is the same as AST.


/** '<<' overloading for the AST structure **/

std::ostream& operator<< (std::ostream& out, AST const& ast);

#endif // AST_H

