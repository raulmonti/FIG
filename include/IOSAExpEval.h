/* Leonardo Rodríguez */
#ifndef IOSAEXPEVAL_H
#define IOSAEXPEVAL_H

#include "IOSAState.h"
#include "ModelAST.h"

namespace iosa {

/**
 * @brief Evaluate expressions according to a given state.
 */
class Evaluator : public Visitor {
private:
    /// The computed value of the expression
    state_value_t value;

    /// The state used to lookup variable values.
    shared_ptr<State> state;

public:
    Evaluator(shared_ptr<State> &state)
        : value {0}, state {state} {}

    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<BinOpExp> node);
    void visit(shared_ptr<UnOpExp> node);

    state_value_t get_value() {
        return (value);
    }
};

}

#endif
