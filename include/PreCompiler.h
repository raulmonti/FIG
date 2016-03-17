#ifndef PRECOMPILER_H
#define PRECOMPILER_H

#include "Ast.h"
#include "Parser.h"
#include <map>

using namespace std;
using namespace parser;

#define GLOBAL_CONST_TABLE Precompiler::get_const_table()

namespace parser{

class Precompiler{

private:

    static map<string,string> mConstTable; // maps constant names into values


    /**
     * @brief Check, solve and fill mConstTable with the values for each
     *        constant definition in @defs.
     */
    void
    solve_constant_defs(vector<AST*> defs, const parsingContext &pc);

    /**
     *
     */
    string
    rec_pre_compile(AST* ast);

public:

/**
 * @brief Ctor.
 */
Precompiler(){};


/**
 * @brief Dtor.
 */
virtual ~Precompiler(){};


/**
 * @brief Get a reference to mConstTable.
 */
inline const static map<string, string>&
get_const_table()
{
    return mConstTable;
}


/**
 * @brief Return a string with the model corresponding to the one in @ast,
 *        but where every constant symbol has been resolved and replaced by its
 *        value, and every constand definition has been replaced by white 
 *        spaces of equal size in number of characters.
 */
string
pre_compile(AST* ast, const parsingContext &pc);


/**
 * @brief Replaces constants in the last parsed model/property file for their
 * values according to the last constants precompilation table.
 * @return String with te result of the replacement.
 */
static string 
replaceConsts(void);

};

} // namespace parser

#endif // PRECOMPILER_H
