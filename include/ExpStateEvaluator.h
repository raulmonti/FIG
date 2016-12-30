/* Leonardo Rodríguez */
#ifndef EXP_STATE_EVAL_H
#define EXP_STATE_EVAL_H

#include <ModelAST.h>
#include <State.h>

//define exprtk_disable_enhanced_features
//define exprtk_enable_debugging
#include <ExpState.h>

#define STYPE STATE_INTERNAL_TYPE
#define NUMTYPE float //exprtk supports float or double.

typedef exprtk::expression<NUMTYPE> expression_t;
typedef exprtk::symbol_table<NUMTYPE> symbol_table_t;

namespace fig {

///@todo template the hell out.
using ExpContainer = std::vector<shared_ptr<Exp>>;
using NameContainer = std::vector<std::string>;
using PositionContainer = std::vector<size_t>;
using ValueContainer = std::vector<NUMTYPE>;

enum class OpKind {
    OP, //e.g 4 + 5
    FUN //e.g min(4,5)
};

/// @brief Translates an AST expression into a ExpTk expression
class ExpTranslatorVisitor : public Visitor {
private:
    std::string exprStr;
    expression_t expr;
    static std::string exprtk_name(ExpOp op);
    static OpKind exprtk_kind(ExpOp op);

public:
    static exprtk::parser<NUMTYPE> parser;

    ExpTranslatorVisitor() {}
    void visit(shared_ptr<IConst> node) noexcept override;
    void visit(shared_ptr<BConst> node) noexcept override;
    void visit(shared_ptr<LocExp> node) noexcept override;
    void visit(shared_ptr<BinOpExp> node) noexcept override;
    void visit(shared_ptr<UnOpExp> node) noexcept override;
    std::string get_string() const noexcept;
    expression_t get_expression(symbol_table_t& table) noexcept;
};

/// @brief Evaluate a vector of expressions using ExprTk
class ExpStateEvaluator {
protected:
    ExpContainer astVec;
    mutable ExpState<NUMTYPE> expState;
    std::vector<expression_t> exprVec;
    symbol_table_t table;
    std::vector<std::string> expStrings;
    size_t numExp;
    bool prepared = false;

public:
    ExpStateEvaluator(const ExpContainer& astVec) noexcept;

    ExpStateEvaluator(shared_ptr<Exp> ast) noexcept :
        ExpStateEvaluator(ExpContainer {ast}) {}

    /// @brief Copy Constructor
    ExpStateEvaluator(const ExpStateEvaluator& that) noexcept;

    ExpStateEvaluator(ExpStateEvaluator&& that) = delete;

    virtual void prepare(const PositionsMap& posMap) noexcept;
    virtual void prepare(const State<STYPE>& state) noexcept;

    bool is_prepared() const noexcept {
        return (prepared);
    }

    STYPE eval(const State<STYPE>& state) const noexcept;
    STYPE eval(const StateInstance& state) const noexcept;
    std::vector<STYPE> eval_all(const State<STYPE>& state) const noexcept;
    std::vector<STYPE> eval_all(const StateInstance& state) const noexcept;

};

} // namespace fig

#endif
