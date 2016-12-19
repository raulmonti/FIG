/* Leonardo Rodríguez */
#ifndef EXP_STATE_EVAL_H
#define EXP_STATE_EVAL_H

#include <ModelAST.h>
#include <State.h>
#define STYPE STATE_INTERNAL_TYPE


namespace fig {

class ExpStatePositionSetter : public Visitor {
    const State<STYPE> *stateptr;
public:
    ExpStatePositionSetter(const State<STYPE>& state) : stateptr {&state} {}
public:
    void visit(shared_ptr<LocExp> node) override;
    void visit(shared_ptr<BinOpExp> node) override;
    void visit(shared_ptr<UnOpExp> node) override;
};

class ExpMapPositionSetter : public Visitor {
    const PositionsMap *posMapPtr;
public:
    ExpMapPositionSetter(const PositionsMap& posMap)
        : posMapPtr {&posMap} {}
public:
    void visit(shared_ptr<LocExp> node) override;
    void visit(shared_ptr<BinOpExp> node) override;
    void visit(shared_ptr<UnOpExp> node) override;
};

class StateEvalVisitor : public Visitor {
    const State<STYPE> *stateptr;
    STYPE value;
public:
    StateEvalVisitor(const State<STYPE> &state) :
        stateptr {&state} {}

    void visit(shared_ptr<IConst> node) override;
    void visit(shared_ptr<BConst> node) override;
    void visit(shared_ptr<FConst> node) override;
    void visit(shared_ptr<LocExp> node) override;
    void visit(shared_ptr<BinOpExp> node) override;
    void visit(shared_ptr<UnOpExp> node) override;
    STYPE get_value();
};

class StateInstanceEvalVisitor : public Visitor {
    const StateInstance *stateptr;
    STYPE value;
public:
    StateInstanceEvalVisitor(const StateInstance &state) :
        stateptr {&state} {}

    void visit(shared_ptr<IConst> node) override;
    void visit(shared_ptr<BConst> node) override;
    void visit(shared_ptr<FConst> node) override;
    void visit(shared_ptr<LocExp> node) override;
    void visit(shared_ptr<BinOpExp> node) override;
    void visit(shared_ptr<UnOpExp> node) override;
    STYPE get_value();
};

///@todo template the hell out.
using ExpContainer = std::vector<shared_ptr<Exp>>;
using NameContainer = std::vector<std::string>;

class ExpStateEvaluator {
protected:
    ExpContainer expVec;
public:
    ExpStateEvaluator(const ExpContainer& expVec) :
        expVec {expVec} {}

    ExpStateEvaluator(shared_ptr<Exp> expr) :
        ExpStateEvaluator(ExpContainer {expr}) {}

    /// @brief Copy Constructor
    ExpStateEvaluator(const ExpStateEvaluator& that) = default;
    ExpStateEvaluator(ExpStateEvaluator&& that) = default;

    virtual void prepare(const PositionsMap& posMap);
    virtual void prepare(const State<STYPE>& state);

    STYPE eval(const State<STYPE>& state) const;
    STYPE eval(const StateInstance& state) const;
    std::vector<STYPE> eval_all(const State<STYPE>& state) const;
    std::vector<STYPE> eval_all(const StateInstance& state) const;    
};

} // namespace fig

#endif
