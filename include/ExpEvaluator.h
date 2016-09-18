/* Leonardo Rodríguez */

#ifndef EXPEVALUATOR_H
#define EXPEVALUATOR_H

#include "ModelTC.h"
#include <functional>

using std::shared_ptr;
using std::make_shared;

/**
 * @brief This visitor reduces expressions to constants.
 * The expressions that can be reduced in compilation time
 * do not depend on state variables, only on global constants
 * and initializations.
 */

class ExpEvaluator : public Visitor {
private:
    /// global constants on which the evaluated expression may depend on
    shared_map<string, Decl>& globals = ModuleScope::globals;
    /// the type of the computed value
    Type type;
    /// the computed value (can be either a boolean, an integer, or a float)
    union value_holder_t {
        bool bvalue;
        int ivalue;
        float fvalue;
    } value;
    /// Evaluate an unary operator.
    void reduce_unary_operator(shared_ptr<UnOpExp> exp);
    /// Evaluate a binary operator.
    void reduce_binary_operator(shared_ptr<BinOpExp> exp);
    /// Signal that the expression is not reducible, for example,
    /// because it depends on state variables.
    void mark_not_reducible();
public:
    ExpEvaluator() : type {Type::tunknown} {}
    /// Interpret a unary operator as a unary function.
    template<typename T>
    static std::function<T (T)> uop_as_fun(ExpOp op);
    /// Interpret a binary operator as a binary function.
    template<typename T>
    static std::function<T (T, T)> bop_as_fun(ExpOp op);
    /// Interpret a binary operator as a relation (comparison)
    template<typename T>
    static std::function<bool (T, T)> bop_as_rel(ExpOp op);
    /// Ask if the value of the expression was obtained properly.
    bool was_reduced();
    /// Interpret the computed value as an integer
    int get_int();
    /// Interpret the computed value as a boolean.
    bool get_bool();
    /// Interpret the computed value as a float.
    float get_float();
    /// Ask if the computed value has type int.
    bool has_type_int();
    /// Ask if the computed value has type bool.
    bool has_type_bool();
    /// Ask if the computed value has type float.
    bool has_type_float();
    /// Returns a string represeting the computed value.
    string value_to_string();
    /// Returns an AST of the computed value.
    /// @note the returned expression should be an IConst, BConst or FConst
    /// class.
    shared_ptr<Exp> value_to_ast();
    /// Reset this object to evaluate another expression
    /// @todo test or remove. Never used.
    void reset();
    /// Visitor functions to evaluate the given expressions.
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<BinOpExp> node);
    void visit(shared_ptr<UnOpExp> node);
};

#endif
