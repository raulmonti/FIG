/* Leonardo Rodríguez */

#ifndef EXPRDNFBUILDER_H
#define EXPRDNFBUILDER_H
#include "ModelAST.h"

using ClauseType = vector<shared_ptr<Exp>>;

/**
 * @brief An expression is in DNF (Disjunctive normal form) if it
 *        has the form
 *        (l_11 && l_12 && ... && l_1m) || ... || (l_n1 && l_n2 && ... && l_nk)
 *        where each l_ij does not contain "&&" or "||"
 * @class ExprClauseBuilder collects the terms "l_ij" in a vector.
 * @class ExprDNFBuilder collects the clauses (l_i1 &&  .... && l_ij) in
 *        a vector (both assuming that the expressions visited *is* in DNF)
 * @see DNFChecker: checks if an expression is in DNF
 */
class ExprClauseBuilder : public Visitor {
    ClauseType clause;
public:
    ExprClauseBuilder() {}
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
    const ClauseType& get_clause() {
        return (clause);
    }
};

class ExprDNFBuilder : public Visitor {
    vector<ClauseType> clause_vector;
public:
    ExprDNFBuilder() {}
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
