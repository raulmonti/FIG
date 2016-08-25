#ifndef EXP_REDUCTOR_H
#define EXP_REDUCTOR_H

#include "ExpEvaluator.h"

class ExpReductor : public Visitor {
private:
    shared_ptr<Exp> reduced_exp = nullptr;
public:
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
    shared_ptr<Exp> get_reduced_exp() {
        return (reduced_exp);
    }
};

#endif

