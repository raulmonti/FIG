#ifndef MODEL_VERIFIER_H
#define MODEL_VERIFIER_H

#include <functional>
#include <set>
#include <memory>

#include "ModelTC.h"
#include <z3++.h>

using std::shared_ptr;

class Z3Converter : public Visitor {
private:
    shared_ptr<z3::context> context;
    z3::expr expression;
    std::set<string> names;
public:
    Z3Converter(const shared_ptr<z3::context> &context)
        : context {context}, expression (*context) {}

    static z3::sort type_to_sort(Type type, z3::context& ctx);
    static std::function<z3::expr (z3::expr)> uop_to_fun(ExpOp op);
    static std::function<z3::expr (z3::expr, z3::expr)> bop_to_fun(ExpOp op);
    std::set<string> get_names();
    z3::expr get_expression();
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
};

class ModelVerifier : public Visitor {
private:
    shared_ptr<z3::context> context;
    shared_ptr<ModuleScope> current_scope = nullptr;
    unique_ptr<z3::solver> solver;
    void add_names_limits(const std::set<string>& names);
    void check_label_preconditions(const string &label_id);
    z3::expr eval_and_convert(shared_ptr<Exp> exp);
    void check_input_determinism();
    unsigned int label_transitions_num(const string &label_id);
    void convert_then_add(shared_ptr<Exp> exp);
public:
    ModelVerifier() {
        context = make_shared<z3::context>();
        solver = make_unique<z3::solver>(*context);
    }
    void visit(shared_ptr<Model> model);
    //void visit(shared_ptr<ModuleBody> node);
};


#endif
