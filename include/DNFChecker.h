/* Leonardo Rodríguez */

#ifndef DNFCHECKER_H
#define DNFCHECKER_H
#include "ModelAST.h"

//Assume TC already done!

class ClauseChecker : public Visitor {
private:
    bool _clause;
public:
    ClauseChecker() : _clause {false} {};
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
    bool is_clause() {
	return _clause;
    }
    ~ClauseChecker() = default;
};

class DNFChecker : public Visitor {
private:
    bool _dnf;
public:
    DNFChecker() : _dnf {false} {};
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
    bool is_dnf() {
	return _dnf;
    }
    ~DNFChecker() = default;
};


#endif
