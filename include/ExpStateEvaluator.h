/* Leonardo Rodríguez */
#ifndef EXP_STATE_EVAL_H
#define EXP_STATE_EVAL_H

#include <ModelAST.h>
#include <State.h>

//define exprtk_disable_enhanced_features
//define exprtk_enable_debugging
#include <ExpState.h>

#define STYPE STATE_INTERNAL_TYPE
#ifndef NTYPE
#  define NTYPE float  //exprtk supports float or double.
#endif

typedef exprtk::expression<NTYPE> expression_t;
typedef exprtk::symbol_table<NTYPE> symbol_table_t;

namespace fig {

///@todo template the hell out.
using ExpContainer = std::vector<shared_ptr<Exp>>;
using NameContainer = std::vector<std::string>;
using PositionContainer = std::vector<size_t>;
using ValueContainer = std::vector<NTYPE>;

enum class OpKind {
    OP, //e.g 4 + 5
    FUN //e.g min(4,5)
};

/// @brief Translates an AST expression into a string parseable
/// by the Exprtk library
class ExpTranslatorVisitor : public Visitor {
private:
    /// The resulting string
    std::string exprStr;

    /// Exprtk specific name for each operator.
    static std::string exprtk_name(ExpOp op);

    /// Indicate if the given operator should be treated  as an
    /// infix operator "e.g 4 + 5" or as a function (prefix) (e.g "max(4,5)")
    static OpKind exprtk_kind(ExpOp op);
public:
    ExpTranslatorVisitor() {}
	inline virtual ~ExpTranslatorVisitor() {}
    void visit(shared_ptr<IConst> node) noexcept override;
    void visit(shared_ptr<BConst> node) noexcept override;
    void visit(shared_ptr<LocExp> node) noexcept override;
    void visit(shared_ptr<BinOpExp> node) noexcept override;
    void visit(shared_ptr<UnOpExp> node) noexcept override;
    std::string get_string() const noexcept;
};

/// @brief Evaluate a vector of expressions using ExprTk
class ExpStateEvaluator {
protected:
    /// The vector of expressions to evaluate
    ExpContainer astVec;

	/// How many expressions do we have
	size_t numExp;

	/// The internal state that contains the values for all the identifiers
    /// occurring on the expressions.
    mutable ExpState<NTYPE> expState;

    /// Vector of Exprtk-expressions to evaluate.
    std::vector<expression_t> exprVec;

    /// Strings that generated our vector of expressions
    std::vector<std::string> expStrings;

    /// Indicate if our internal state has already been syncronized with the main
    /// simulation state
    bool prepared = false;

    ///
    /// \brief valuation results of valuating all our expresions in a state
    ///
    mutable std::vector<STYPE> valuation;

public:
	ExpStateEvaluator(const ExpContainer& astVector);

	ExpStateEvaluator(shared_ptr<Exp> ast) :
		ExpStateEvaluator(ExpContainer {ast}) {}

    /// @brief Copy Constructor
	ExpStateEvaluator(const ExpStateEvaluator& that);

	ExpStateEvaluator(ExpStateEvaluator&& that);

	inline virtual ~ExpStateEvaluator() {}

    /// Associate our internal state with the main simulation state.
    /// @see ExpState::project_positions
    /// @note this must be called before \ref eval
    virtual void prepare(const PositionsMap& posMap) noexcept;
    virtual void prepare(const State<STYPE>& state) noexcept;

    /// Was \ref prepare called?
    bool is_prepared() const noexcept {
        return (prepared);
    }

    /// How many expressions do we have to evaluate?
    size_t number_of_expressions() const noexcept {
        return (astVec.size());
    }

    /// Update our internal state and then evaluate all the expressions
    /// @returns a vector with the results for each expression
    std::vector<STYPE>& eval_all(const State<STYPE>& state) const noexcept;
    std::vector<STYPE>& eval_all(const StateInstance& state) const noexcept;

    /// Evaluate all the expressions but return the result of the first one.
    STYPE eval(const State<STYPE>& state) const noexcept;
    STYPE eval(const StateInstance& state) const noexcept;

	/// Vector with strings for all internal expressions
	inline const std::vector<std::string>& to_string() const noexcept
		{ return expStrings; }
};

} // namespace fig

#endif
