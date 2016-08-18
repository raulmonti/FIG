/* Leonardo Rodríguez */

#ifndef EXPRDNFBUILDER_H
#define EXPRDNFBUILDER_H
#include "ModelAST.h"

//Assumes TC already done!

using ClauseType = vector<shared_ptr<Exp>>;

class ExprClauseBuilder : public Visitor {
    ClauseType clause;
public:
    ExprClauseBuilder() {};
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
    const ClauseType &get_clause() {
        return (clause);
    }
};

class ExprDNFBuilder : public Visitor {
    vector<ClauseType> clause_vector;
public:
    ExprDNFBuilder() {};
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
    const vector<ClauseType>& get_clauses() {
        return clause_vector;
    }
};


#endif
