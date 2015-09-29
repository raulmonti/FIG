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
    vector<AST*>    branches;       // branches of children
 
    // @Constructor.
    AST(void);
    AST(int token = 0, string lexeme = "", int line = 0, int col = 0);

    // @Destructor.
    virtual ~AST();

    // @Push back a new child into @l.
    inline void pb(AST *c)
    {
        branches.push_back(c);
    }

    /* Human readable string version of this AST Node (without childs). */
    string
    p_node(void);

    /* Printable position. */
    string
    p_pos();

    /* Printable name. */ //FIXME comment this methods correctly
    string
    p_name();

    /* @get_list: get a vector with pointers to every child AST with token k.
    */
    vector<AST*>
    get_list(int k);


    /* @get_first: walk the tree and get the first node with token k.
       @return: The found node or NULL otherwise.     
    */
    AST*
    get_first(int k);


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

    /* @get_all_ast: walk the tree and get every node with token k.
       @return: a vector of AST pointers to every node with token k.     
    */
    vector<AST*>
    get_all_ast(int k);

    /* @get_all_ast_ff: (get all ast first found) walk the tree and get every
                        node with token k, but stop deepening into the branch
                        as soon as we get such a node. Then searching K2 in a 
                        tree K1 [k2 [K2, K3], K4 []] will only return the
                        first K2.
       @return: a vector of AST pointers to every node with token k, first
                found.
    */
    vector<AST*>
    get_all_ast_ff(int k);

    /* @get_child: Get the ith branch. */
    AST*
    get_branch(int i);

    /* @get_branch_k: get first child AST with token k.
       @return: pointer to the found AST, NULL otherwise.
    */
    AST*
    get_branch_k(int k);

    /*  @get_line: get a string with the line number of this AST.
    */
    string 
    get_line();

    /*  @get_column: get a string with the column number of this AST.
    */
    string 
    get_column();

};


typedef AST Node; // Node is the same as AST.


/** '<<' overloading for the AST structure **/

std::ostream& operator<< (std::ostream& out, AST const& ast);




#endif // AST_H

