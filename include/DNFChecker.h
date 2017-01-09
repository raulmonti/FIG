/* Leonardo Rodríguez */

#ifndef DNFCHECKER_H
#define DNFCHECKER_H
#include "ModelAST.h"

//Assume TC already done!

/**
 * @brief Some visitors to check if an expression is in Disjuntive Normal Form
 */

class ClauseChecker : public Visitor {
private:
    /// Is the expression a "clause",
    /// i.e it has the form : (b1 & .... & bn) where each bN does not have disjunctions
    bool _clause;
public:
    ClauseChecker() : _clause {false} {};
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<BinOpExp> node);
    void visit(shared_ptr<UnOpExp> node);
    bool is_clause() {
	return _clause;
    }
    ~ClauseChecker() = default;
};

class DNFChecker : public Visitor {
private:
    /// Is the expression in "dnf":
    /// i.e it has the form: "c1 | ... | cn" where each cN is a clause
    bool _dnf;
public:
    DNFChecker() : _dnf {false} {};
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<BinOpExp> node);
    void visit(shared_ptr<UnOpExp> node);
    bool is_dnf() {
	return _dnf;
    }
    ~DNFChecker() = default;
};


#endif
