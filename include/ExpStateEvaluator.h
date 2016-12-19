/* Leonardo Rodríguez */
#ifndef EXP_STATE_EVAL_H
#define EXP_STATE_EVAL_H

#include <ModelAST.h>
#include <State.h>
#define STYPE STATE_INTERNAL_TYPE


namespace fig {

class ExpNamesCollector : public Visitor {
private:
    std::vector<std::string> names;
public:
    void visit(shared_ptr<LocExp> node) override;
    void visit(shared_ptr<BinOpExp> node) override;
    void visit(shared_ptr<UnOpExp> node) override;

    std::vector<std::string> get_names() const {
        return (names);
    }
};

///@todo template the hell out.
using ExpContainer = std::vector<shared_ptr<Exp>>;
using NameContainer = std::vector<std::string>;

class ExpStateEvaluator {
protected:
    ExpContainer expVec;
    size_t varNum;
    NameContainer varNames;
    // given a variable name give me an index to lookup in the
    // vectors below
    PositionsMap positionOf;
    // position in the state
    std::vector<size_t> statePositions;
    // value in the state
    mutable std::vector<STYPE>  stateValues;

public:
    ExpStateEvaluator(const ExpContainer& expVec) :
        expVec {expVec} {
        for (shared_ptr<Exp> exp : expVec) {
            ExpNamesCollector visitor;
            exp->accept(visitor);
            const std::vector<std::string>& names = visitor.get_names();
            varNames.insert(varNames.end(), names.cbegin(), names.cend());
        }
        varNum = varNames.size();
        statePositions.resize(varNum);
        stateValues.resize(varNum);
    }

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

private:
    class EvalVisitor : public Visitor {
        STYPE value = 0;
        const std::vector<STYPE>& stateValues;
        const PositionsMap& positionOf;

    public:
        EvalVisitor(const std::vector<STYPE> &values,
                    const PositionsMap& positionMap)
            : stateValues {values}, positionOf {positionMap} {}

        void visit(shared_ptr<IConst> node) override;
        void visit(shared_ptr<BConst> node) override;
        void visit(shared_ptr<FConst> node) override;
        void visit(shared_ptr<LocExp> node) override;
        void visit(shared_ptr<BinOpExp> node) override;
        void visit(shared_ptr<UnOpExp> node) override;
        STYPE get_value();
    };
};

} // namespace fig

#endif
