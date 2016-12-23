/* Leonardo Rodríguez */
#ifndef EXP_STATE_EVAL_H
#define EXP_STATE_EVAL_H

#include <ModelAST.h>
#include <State.h>

//#define exprtk_disable_enhanced_features
//#define exprtk_enable_debugging
#include "exprtk.hpp" // 1.3mb

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
    void visit(shared_ptr<IConst> node) override;
    void visit(shared_ptr<BConst> node) override;
    void visit(shared_ptr<LocExp> node) override;
    void visit(shared_ptr<BinOpExp> node) override;
    void visit(shared_ptr<UnOpExp> node) override;
    std::string get_string() const;
    expression_t get_expression(symbol_table_t& table);
};


/// @brief Collects every variable name occuring on the AST into a vector
class ExpNameCollector: public Visitor {
    NameContainer names;
public:
    ExpNameCollector() {}
    void visit(shared_ptr<LocExp> node) override;
    void visit(shared_ptr<BinOpExp> node) override;
    void visit(shared_ptr<UnOpExp> node) override;
    inline NameContainer get_names() {
        return (names);
    }
};

/// @brief Local state used to evaluate the expressions. Regularly updated
/// using an external state.
class ExpInternalState {
    // grant access to vectors:
    friend class ExpTableFiller;
    friend class ExpStateEvaluator;
private:
    // varNames: "x" "y" "z"
    // varPos:   "1" "3" "4"
    // varValues: 4  17  23
    // represents the state {x->4, y->17, z->23}
    // what will be updated to {x->ST(1), y->ST(3), z->ST(4)}
    // during evaluation with a given state ST.
    NameContainer varNames;
    PositionContainer varPos;
    ValueContainer varValues;
protected:
    size_t get_local_position_of(const std::string &name) const;

public:
    ExpInternalState() {}
    void add_variables(const ExpContainer &astVec);
    void project_positions(const State<STYPE> &state);
    void project_positions(const PositionsMap &posMap);
    void project_values(const State<STYPE> &state);
    void project_values(const StateInstance &state);
};

/// @brief Fill a ExprTk symbol table with mappings of the form
/// [variable->position in local state]
class ExpTableFiller : public Visitor {
private:
    symbol_table_t &table;
    ExpInternalState &expState;

public:
    ExpTableFiller(symbol_table_t &table, ExpInternalState &expState)
        : table {table}, expState {expState} {}

    void visit(shared_ptr<LocExp> node) override;
    void visit(shared_ptr<BinOpExp> node) override;
    void visit(shared_ptr<UnOpExp> node) override;
};

/// @brief Evaluate a vector of expressions using ExprTk
class ExpStateEvaluator {
private:
    mutable ExpInternalState expState;
protected:
    ExpContainer astVec;
    std::vector<expression_t> exprVec;
    symbol_table_t table;
    std::vector<std::string> expStrings;

public:
    ExpStateEvaluator(const ExpContainer& astVec);

    ExpStateEvaluator(shared_ptr<Exp> ast) :
        ExpStateEvaluator(ExpContainer {ast}) {}

    /// @brief Copy Constructor
    ExpStateEvaluator(const ExpStateEvaluator& that);

    ExpStateEvaluator(ExpStateEvaluator&& that) = delete;

    virtual void prepare(const PositionsMap& posMap);
    virtual void prepare(const State<STYPE>& state);

    STYPE eval(const State<STYPE>& state) const;
    STYPE eval(const StateInstance& state) const;
    std::vector<STYPE> eval_all(const State<STYPE>& state) const;
    std::vector<STYPE> eval_all(const StateInstance& state) const;

};

} // namespace fig

#endif
