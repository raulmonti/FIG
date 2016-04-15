//==============================================================================
//
//    Abstract syntax tree module
//    Raul Monti
//    2015
//
//==============================================================================

#ifndef AST_H
#define AST_H

#include <vector>
#include <string>
#include <sstream>

using std::vector;
using std::string;
using std::stringstream;



//==============================================================================
// The Abstract Syntax Tree class ==============================================
//==============================================================================


typedef int Key;


class AST
{
public:

    string          lxm;        // lexeme
    int             tkn;        // token
    int             l;          // line number
    int             c;          // column number
    vector<AST*>    branches;   // branches of children
 
    /**
     * @brief: Constructor.
     */
    AST(void);

    /**
     * @brief: Constructor.
     */
    AST(int token = 0, string lexeme = "", int line = 0, int col = 0);

    /**
     * @brief: Constructor.
     */
    AST(const AST* copy);

    /**
     * @brief: Destroyer.
     */
    virtual ~AST();

    /**
     * @brief: back a new child into @l.
     */
    inline void pb(AST *c)
    {
        branches.push_back(c);
    }

    /**
     * @brief: Human readable string version of this AST Node (without childs).
     */
    string
    p_node(void);

    /**
     * @deprecated: use get_pos() instead.
     * @brief: Printable position.
     */
    string
    p_pos();

    /**
     * @brief: Printable name.
     */
    string
    p_name();

    /**
     * @brief: get this AST branches with token @k. 
     */
    vector<AST*>
    get_list(int k);


    /** 
     * @brief: walk the tree and get the first node with token @k.
     * @return: The found node or NULL no node with token @k can be found.     
     */
    AST*
    get_first(int k);

    /**
     * @brief: get a list with the lexemes of every branch with token @k.
     */ 
    vector<string>
    get_list_lexemes(int k);

    /** 
     * @brief: Get the lexeme of the first child AST with token @k.
     * @return: "" if not found, the lexeme as a string otherwise.
     */
    string
    get_lexeme(int k);

    /**
     * @brief: recursively retrieve every lexeme from every node with token @k.
     */
    vector<string>
    get_all_lexemes(int k);

    /**
     * @brief: walk the tree and get every node with token @k.
     * @return: a vector of AST pointers to every node with token @k found.     
     */
    vector<AST*>
    get_all_ast(int k);

    /**
     * @brief: (get all ast first found) walk the tree and get every
     *         node with token k, but stop deepening into the branch
     *         as soon as we get such a node. Then searching K2 in a tree 
     *         K1 [k2 [K2, K3], K4 []] will only return the first K2.
     * @return: a vector of AST pointers to every node with token @k, first 
     *          found.
     */
    vector<AST*>
    get_all_ast_ff(int k);

    /**
     *  @brief: Get the i-th branch.
     */
    AST*
    get_branch(int i);

    /** @brief: get first child AST with token @k.
     *  @return: pointer to the found AST, NULL otherwise.
     */
    AST*
    get_branch_k(int k);

    /**
     * @brief: get a string with the line number of this AST instance.
     */
    string 
    get_line();

    /**
     * @brief: get a string with the line number of this AST instance.
     */
    string 
    get_column();

    /** 
     * @brief: get the line and column of this AST instance, separated by a 
     *         colon.
     */
    string
    get_pos();

    /**
     * TODO
     */
    string
    toString() const;

    /**
     * TODO
     */
    friend bool
    operator==(AST &ast1,AST &ast2);

    /**
     * TODO
     */
    friend bool
    operator!=(AST &ast1,AST &ast2);
};


typedef AST Node; // Node is the same as AST.


//==============================================================================
// '<<' overloading for the AST structure ======================================
//==============================================================================

std::ostream& operator<< (std::ostream& out, AST const& ast);


#endif // AST_H

