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
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<BinOpExp> node);
    void visit(shared_ptr<UnOpExp> node);

    std::vector<std::string> get_names() const {
        return (names);
    }
};


class ExpStateEvaluator {
private:

    shared_ptr<Exp> expr;
    size_t varNum;
    std::vector<std::string> varNames;
    // given a variable name give me an index to lookup in the
    // vectors below
    std::unordered_map<std::string, size_t> positionOf;
    // position in the state
    std::vector<size_t> statePositions;
    // value in the state
    std::vector<STYPE>  stateValues;

public:

    ExpStateEvaluator(std::shared_ptr<Exp> expr) : expr {expr} {
        ExpNamesCollector visitor;
        expr->accept(visitor);
        varNames = visitor.get_names();
        varNum = varNames.size();
        statePositions.reserve(varNum);
        stateValues.reserve(varNum);
    }

    /// @brief Copy Constructor
    ExpStateEvaluator(const ExpStateEvaluator& that) = default;
    ExpStateEvaluator(ExpStateEvaluator&& that) = default;

    virtual void prepare(const PositionsMap& posMap);
    virtual void prepare(const State<STYPE>& state);
    STYPE eval(const State<STYPE>& state) const;
    STYPE eval(const StateInstance& state) const;


private:
    class EvalVisitor : public Visitor {
        STYPE value = 0;
    public:
        void visit(shared_ptr<IConst> node);
        void visit(shared_ptr<BConst> node);
        void visit(shared_ptr<FConst> node);
        void visit(shared_ptr<LocExp> node);
        void visit(shared_ptr<BinOpExp> node);
        void visit(shared_ptr<UnOpExp> node);
        STYPE get_value();
    };
};

} // namespace fig

#endif
