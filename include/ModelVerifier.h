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

class Z3Converter : public Visitor {
private:
    shared_ptr<z3::context> context;
    z3::expr expression;
    std::set<string> names;
    std::map<string, z3::sort> sorts;
public:
    Z3Converter(const shared_ptr<z3::context> &context)
        : context {context}, expression (*context) {}
    static z3::sort type_to_sort(Type type, z3::context& ctx);
    static z3unaryfun uop_to_fun(ExpOp op);
    static z3binaryfun bop_to_fun(ExpOp op);
    std::set<string> get_names();
    z3::expr get_expression();
    z3::sort get_sort_of(const string &name);
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
};

//condition 1 y 2: enforced by parser
//condition 3: check_output_determinism_all
//condition 4: missing
//condition 5 y 6: checked by back-end
//condition 7: check_input_determinism_all

class ModelVerifier : public Visitor {
private:
    shared_ptr<z3::context> context;
    shared_ptr<ModuleScope> current_scope = nullptr;
    unique_ptr<z3::solver> solver;
    void add_names_limits(const std::set<string>& names);
    z3::expr eval_and_convert(shared_ptr<Exp> exp);
    //condition 3:
    void check_output_determinism(const string &clock_id);
    void check_output_determinism_all();
    //condition 7:
    void check_input_determinism(const string &label_id);
    void check_input_determinism_all();
    unsigned int label_transitions_num(const string &label_id);
    z3::expr convert(shared_ptr<Exp> exp);
    z3::expr convert(shared_ptr<Exp> exp, std::set<string>& names);
    void check_rhs(shared_ptr<Action> a1, shared_ptr<Action> a2);
    void debug_print_solver();
    z3::expr convert_and_rename(shared_ptr<Exp> expr,
                                std::set<string> &names_set);
    void add_assignments_as_equalities(shared_vector<Effect> effects);
public:
    ModelVerifier() {
        context = make_shared<z3::context>();
        solver = make_unique<z3::solver>(*context);
    }
    void visit(shared_ptr<Model> model);
    //void visit(shared_ptr<ModuleBody> node);
};


#endif
