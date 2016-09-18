#ifndef EXP_REDUCTOR_H
#define EXP_REDUCTOR_H

#include "ExpEvaluator.h"

/**
 * @brief Traverse the AST of an expression
 *        building a copy of it that has been
 *        reduced as much as posible
 * @example For (q + L == 2 + 2) this will build (q + 5 == 4)
 *          when L = 5 is a constant in the global scope.
 */
class ExpReductor : public Visitor {
private:
    /// The reduced expression.
    shared_ptr<Exp> reduced_exp = nullptr;
public:
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<BinOpExp> node);
    void visit(shared_ptr<UnOpExp> node);

    shared_ptr<Exp> get_reduced_exp() {
        return (reduced_exp);
    }
};

#endif

