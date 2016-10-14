/* Leonardo Rodríguez */

#ifndef EXPRDNFBUILDER_H
#define EXPRDNFBUILDER_H

#include "ModelAST.h"
#include "ModuleScope.h"

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
    /// ModuleScope
    shared_ptr<ModuleScope> scope = nullptr;
    /// A vector of terms (expressions), each term represents
    /// a "l_ij" as explained before.
    ClauseType clause;
public:
    ExprClauseBuilder(shared_ptr<ModuleScope> scope) : scope {scope} {}
    /// Visitor functions to build the vector of terms
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<BinOpExp> node);
    void visit(shared_ptr<UnOpExp> node);
    const ClauseType& get_clause() {
        return (clause);
    }
private:
    shared_ptr<Exp> reduce_exp(shared_ptr<Exp> exp);
};

class ExprDNFBuilder : public Visitor {
    /// ModuleScope
    shared_ptr<ModuleScope> scope = nullptr;
    /// Vector of clauses of the form (l_n1 && ... && l_nk) as explained
    /// above.
    vector<ClauseType> clause_vector;
public:
    ExprDNFBuilder(shared_ptr<ModuleScope> scope) : scope {scope} {}
    /// Visit each constant, expression, operator in order to build the
    /// above vector.
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<BinOpExp> node);
    void visit(shared_ptr<UnOpExp> node);
    const vector<ClauseType>& get_clauses() {
        return clause_vector;
    }
private:
    shared_ptr<Exp> reduce_exp(shared_ptr<Exp> exp);
};


#endif
