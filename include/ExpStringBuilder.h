#ifndef EXP_STRING_BUILDER_H
#define EXP_STRING_BUILDER_H

#include "ModelAST.h"
#include "ModuleScope.h"

/**
 * @brief Traverse the AST of an expression building a string
 *        representation and a vector of variables occurring on it.
 */
class ExpStringBuilder : public Visitor {
    /// ModuleScope used to evaluate state variables.
    shared_ptr<ModuleScope> scope = nullptr;
    /// state variables occuring in the expression
    vector<std::string> names;
    /// the computed string
    std::string result;
    // @todo:
    // Instead, I think we should add parenthesis and implement a (probably complicated)
    // function that removes parenthesis taking into account the precedence and
    // associativity of the operators. For example in ((a * b) + c) + d no
    // parenthesis is really necessary since * has a bigger precedence than
    // + and also + is left associative.

public:
    ExpStringBuilder(shared_ptr<ModuleScope> scope)
        : scope {scope}, result {""} {}
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<BinOpExp> node);
    void visit(shared_ptr<UnOpExp> node);
    /// Return the vector of state variables of this expression
    const vector<std::string>& get_names();

    std::string str();
};


#endif
