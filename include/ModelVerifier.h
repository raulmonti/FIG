#ifndef MODEL_VERIFIER_H
#define MODEL_VERIFIER_H

#include <functional>
#include <set>
#include <memory>

#include "ModelTC.h"
#include <z3++.h>

using std::shared_ptr;
using z3binaryfun = std::function<z3::expr (z3::expr, z3::expr)>;
using z3unaryfun = std::function<z3::expr (z3::expr)>;

/**
 * @brief Converts an AST expression into a z3::expr
 */
class Z3Converter : public Visitor {
private:
    /// The context of the expression.
    shared_ptr<z3::context> context;
    /// The expression itself
    z3::expr expression;
    /// The names of the state-variables that occur in the expression
    std::set<string> names;
    /// The sort of each stata-variable (int, float or bool)
    std::map<string, z3::sort> sorts;
public:
    Z3Converter(const shared_ptr<z3::context> &context)
        : context {context}, expression (*context) {}
    /// Interpret a type as a sort
    static z3::sort type_to_sort(Type type, z3::context& ctx);
    /// Interpret an unary operator as a z3 unary function
    static z3unaryfun uop_to_fun(ExpOp op);
    /// Interpret an binary operator as a z3 binary function
    static z3binaryfun bop_to_fun(ExpOp op);
    /// Return the state-variables that occur in the expression.
    std::set<string> get_names();
    /// Return the expression built during visitation
    z3::expr get_expression();
    /// Return the sort of given state-variable
    z3::sort get_sort_of(const string &name);
    /// Visitor functions
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
};

/**
 * @brief Verify that some IOSA-conditions holds in the
 *        AST model.
 * @note Condition 1: "no clock for input labels" => ensured by parser
 *       Condition 2: "unique clock for output labels" => ensured by parser
 *       Condition 3: @see ModelVerifier::check_output_determinism_all
 *       Condition 4: @see ModelVerifier::check_exhausted_clocks_all
 *       Condition 5: "initially reseted clocks" => ensured by backend
 *       Condition 6: "general input-enabled" => ensured by backend
 *       Condition 7: @see ModelVerifier::check_input_determinism_all
 * @note Checking are done without "reachability analisis"
 */
class ModelVerifier : public Visitor {
private: // Members
    /// Context used in every z3::expr involved in the verification
    shared_ptr<z3::context> context;

    /// Symbol table of the module being verified
    shared_ptr<ModuleScope> current_scope = nullptr;

    /// Z3 solver used to check that the conditions hold.
    unique_ptr<z3::solver> solver;

private: // Auxiliar functions
    /// Adds to the solvers assertions to ensure that the
    /// state-variables take values inside the allowed range
    void add_names_limits(const std::set<string>& names);

    /// Tries to evaluate the expressions (reduce it into a constant)
    /// and convert the result into a z3::expr.
    /// @throws FigException if could not evaluate the expression
    z3::expr eval_and_convert(shared_ptr<Exp> exp);

    /// Converts the AST expression into a z3::expression
    z3::expr convert(shared_ptr<Exp> exp);

    /// Converts the AST expression into a z3::expression and stores the
    /// state variables that occur in the expression inside the given set.
    z3::expr convert(shared_ptr<Exp> exp, std::set<string>& names);

    /// Check if the postconditions of the given transitions are equivalent
    /// (i.e, each state-variable changes in the same way in both transitions)
    void check_rhs(shared_ptr<Action> a1, shared_ptr<Action> a2);

    /// Prints the content of the z3 solver for debugging purposes
    void debug_print_solver();

    /// Converts the given expression into a z3::expr, then rename
    /// those state-variables that occur in the given expression and
    /// also occur in the the given set, inserting a character ' at
    /// the end of the name.
    /// @example if the expression is "q > r + p" and the given
    /// set is {q, r}, the resulting expresion will be "q' > r' + p"
    /// note that "p" is not renamed.
    z3::expr convert_and_rename(shared_ptr<Exp> expr,
                                std::set<string> &to_rename_vars);

    /// Add to the solver equalities that express the assignments in
    /// the given effects, store the changed state varibles inside the
    /// given set.
    /// @example if the effects are "q' = q + 1 & p' = p + q"
    ///          the equalities added are "q' == q + 1" and "p' == p + q"
    ///          and the variables "q,p" are added to the "changed_names" set.
    void add_assignments_as_equalities(const shared_vector<Effect>& effects,
                                       std::set<string> &changed_names);

    /// Returns a z3::expression with the OR of all the preconditions
    /// of all the transition in this module that wait for the given clock.
    /// @example [a!] b1 @ c1 -> e1 ;
    ///          [b!] b2 @ c2 -> e2 ;
    ///          [c?] b3 -> e3;
    ///          [d!] b4 @ c1 -> e4;
    /// if clock_id == c1, then this function returns "b1 || b4"
    z3::expr pre_transitions_with_clock(const string &clock_id,
                                        std::set<string> &names);

    /// This function returns true iff there is a valuation for state
    /// variables (inside the corresponding range) such that:
    /// 1 - the precondition of "a2" holds.
    /// 2 - the precondition of "a1" does not hold.
    /// 3 - the precondition of "a1" holds after "a2" modifies the state.
    /// 4 - there is no transition waiting for "clock_id" whose precondition
    /// hold, or "a2" waits for "clock_id".
    /// That means that a1 is enables by a2 with a potentially exhausted clock
    bool enables_exhausted(shared_ptr<Action> a1, shared_ptr<Action> a2,
                           const string &clock_id);

private:
    /// Check condition 3 of IOSA.
    /// Let [a!] b1 @ c -> e1 and [b!] b2 @ c -> e2 be transitions waiting
    /// for the same clock "c".
    /// If b1 & b2 hold => a=b & reseted_clocks(e1) = reseted_clocks(e2) &
    ///                    & same_state(e1, e2)
    /// That means that if two transitions are enabled with the same clock,
    /// there is no observable difference if one or the other transition is
    /// executed.
    void check_output_determinism(const string &clock_id);
    void check_output_determinism_all();
private:
    /// Check condition 7 of IOSA.
    /// Let [a?] b1 -> e1 and [a?] b2 -> e2 be transitions with the same
    /// input label "a".
    /// If b1 & b2 hold => reseted_clocks(e1) = reseted_clocks(e2)
    ///                    & same_state(e1, e2)
    void check_input_determinism(const string &label_id);
    void check_input_determinism_all();
private:
    /// Check condition 4 of IOSA
    /// Let P(T) be the precondition of transition T
    /// Let Q(T) be the postcondition of transition T (a state transformation)
    /// Let W(T) be the clock on which T is waiting for.
    /// Let R(T) be the clocks that T resets.
    /// Check that:
    ///   FORALL T1 : output transition of the current module
    ///   FORALL T2 : transition of the current module
    ///        W(T1) belongs to R(T1) or
    ///        There is NOT a valid valuation "v" of state-variables
    ///        such that
    ///        P(T2)v & !P(T1)v & P(T1)(Q(T2)v) &
    ///        (!OR(W(T1))v || W(T2) = W(T1))
    /// Here OR(W(T1)) is the OR of the preconditions of the transitions
    /// waiting for W(T1)
    /// @see enables_exhausted
    void check_exhausted_clocks(const string &clock_id);
    void check_exhausted_clocks_all();
public:
    ModelVerifier() {
        context = make_shared<z3::context>();
        solver = make_unique<z3::solver>(*context);
    }
    /// The visitor function
    void visit(shared_ptr<Model> model);
};


#endif
