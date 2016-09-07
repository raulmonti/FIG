/* Leonardo Rodríguez */

#include <cassert>
#include <sstream>
#include <functional>

#include "ModelTC.h"
#include "ModelPrinter.h"
#include "DNFChecker.h"
#include "location.hh"
#include "ExpEvaluator.h"

using std::endl;
using std::shared_ptr;
using std::make_pair;

/* Typechecking on Model */

//initialize static maps
shared_map<string, ModuleScope> ModuleScope::scopes;
shared_map<string, Decl> ModuleScope::globals;

//Some error messages:
namespace {

stringstream& operator<<(stringstream &ss, ModelAST model) {
    ss << " [at " << *(model.get_location()) << "]";
    return ss;
}

inline const string UNEXPECTED_TYPE(Type expected, Type got) {
    return "Expected type is " + ModelPrinter::to_str(expected) +
            " - Inferred type is " + ModelPrinter::to_str(got);
}

inline const string EXPECTED_NUMERIC() {
    return "Expected type " + ModelPrinter::to_str(Type::tint) +
            " or type " + ModelPrinter::to_str(Type::tfloat);
}

inline const string PREFIX(const shared_ptr<ModuleScope>& current_scope) {
    return current_scope == nullptr ? "At global constants" :
                                      "At Module " + current_scope->id;
}

inline const string TC_WRONG_INDEX_INT(const shared_ptr<ModuleScope> &curr,
                                       const shared_ptr<Location> &loc,
                                       const shared_ptr<Exp> &exp,
                                       Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Identifier";
    ss << " \"" << loc->id << "\"";
    ss << *loc;
    ss << " - The expression for the index";
    ss << *exp;
    ss << " is ill-typed";
    ss << " - " << UNEXPECTED_TYPE(Type::tint, last_type);
    return (ss.str());
}

inline const string TC_ID_REDEFINED(const shared_ptr<ModuleScope> &curr,
                                    const shared_ptr<ModelAST> &prev,
                                    const string &id) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Identifier";
    ss << " \"" << id << "\"";
    ss << " is already declared";
    ss << " at " << *(prev->get_location());
    return (ss.str());
}

inline const string TC_ID_OUT_OF_SCOPE(const shared_ptr<ModuleScope> &curr,
                                       const shared_ptr<Location> &loc) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Identifier";
    ss << " \"" << loc->id << "\"";
    ss << " [at " << *(loc->get_location()) << "]";
    ss << "is not in scope";
    return (ss.str());
}

inline const string TC_WRONG_LOWER_BOUND(
        const shared_ptr<ModuleScope>& curr,
        const string &id,
        const shared_ptr<Exp> &exp,
        const Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Identifier";
    ss << " \"" << id << "\"";
    ss << " - Expression of lower bound";
    ss << *exp;
    ss << " is ill-typed";
    ss << " - " << UNEXPECTED_TYPE(Type::tint, last_type);
    return (ss.str());
}

inline const string TC_WRONG_UPPER_BOUND(
        const shared_ptr<ModuleScope>& curr,
        const string &id,
        const shared_ptr<Exp> &exp,
        const Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Identifier";
    ss << " \"" << id << "\"";
    ss << " - Expression of upper bound";
    ss << *exp;
    ss << " is ill-typed";
    ss << " - " << UNEXPECTED_TYPE(Type::tint, last_type);
    return (ss.str());
}

inline const string TC_WRONG_SIZE_EXP(const shared_ptr<ModuleScope>& curr,
                                      const string &id,
                                      const shared_ptr<Exp> &exp,
                                      const Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Identifier";
    ss << " \"" << id  << "\"";
    ss << " - Array size expression is ill typed";
    ss << *exp;
    ss << " - " << UNEXPECTED_TYPE(Type::tint, last_type);
    return (ss.str());
}

inline const string TC_WRONG_DIST_FST_PARAM(
        const shared_ptr<ModuleScope> &curr,
        const shared_ptr<Dist> &dist,
        const Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Distribution ";
    ss << ModelPrinter::to_str(dist->type);
    ss << *dist;
    ss << " - First parameter is ill typed";
    ss << " - " << UNEXPECTED_TYPE(Type::tfloat, last_type);
    return (ss.str());
}

inline const string TC_WRONG_DIST_SND_PARAM(
        const shared_ptr<ModuleScope> &curr,
        const shared_ptr<Dist> &dist,
        const Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Distribution ";
    ss << ModelPrinter::to_str(dist->type);
    ss << *dist;
    ss << " - Second parameter is ill typed";
    ss << " - " << UNEXPECTED_TYPE(Type::tfloat, last_type);
    return (ss.str());
}

inline const string TC_WRONG_INIT_EXP(const shared_ptr<ModuleScope>& curr,
                                      const string &id,
                                      const shared_ptr<Exp> &init,
                                      const Type expected,
                                      const Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Identifier";
    ss << "\"" << id  << "\"";
    ss << " - Initialization is ill-typed";
    ss << *init;
    ss << " - " << UNEXPECTED_TYPE(expected, last_type);
    return (ss.str());
}

inline const string TC_MULTIPLE_LABEL_TYPE(
        const shared_ptr<ModuleScope>& curr,
        const string &label) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Label";
    ss << " \"" << label << "\"";
    ss << " must have a single type";
    return (ss.str());
}

inline const string TC_MULTIPLE_CLOCK_DIST(
        const shared_ptr<ModuleScope> &curr,
        const shared_ptr<Dist> &dist1,
        const shared_ptr<Dist> &dist2,
        const string &clock_id) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Clock";
    ss << " \"" << clock_id << "\"";
    ss << " must have a single distribution type";
    ss << " - reseted as " << ModelPrinter::to_str(dist1->type);
    ss << *dist1;
    ss << " and reseted as " << ModelPrinter::to_str(dist2->type);
    ss << *dist2;
    return (ss.str());
}

inline const string TC_MISSING_CLOCK_DIST(
        const shared_ptr<ModuleScope> &curr,
        const string &clock_id,
        const shared_ptr<Decl> decl) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Clock";
    ss << " \"" << clock_id << "\"";
    ss << *decl;
    ss << " must have a distribution type";
    return (ss.str());
}

inline const string TC_NOT_A_CLOCK(
        const shared_ptr<ModuleScope> &curr,
        const shared_ptr<Action> &action,
        Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Transition of label";
    ss << " \"" << action->id << "\"";
    ss << *action;
    ss << " - Identifier";
    ss << " \"" << action->clock_loc->id << "\"";
    ss << " is not a clock";
    ss << " - " << UNEXPECTED_TYPE(Type::tclock, last_type);
    return (ss.str());
}

inline const string TC_WRONG_PRECONDITION(
        const shared_ptr<ModuleScope>& curr,
        const shared_ptr<Action>& action,
        Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Transition of label";
    if (action->type == LabelType::empty) {
        ss << " tau";
    } else {
        ss << " \"" << action->id << "\"";
    }
    ss << " - Precondition is ill-typed";
    ss << *(action->guard);
    ss << " - " << UNEXPECTED_TYPE(Type::tbool, last_type);
    return (ss.str());
}

inline const string TC_WRONG_RHS(const shared_ptr<ModuleScope> &curr,
                                 const shared_ptr<Effect> &effect,
                                 Type expected,
                                 Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Assignment of state variable";
    ss << " \"" << effect->loc->id << "\"";
    ss << " - Right-hand side expression is ill-typed";
    ss << *(effect->arg);
    ss << " - " << UNEXPECTED_TYPE(expected, last_type);
    return (ss.str());
}

inline const string TC_WRONG_FST_ARG(
        const shared_ptr<ModuleScope> &curr, ExpOp op,
        const shared_ptr<Exp> &exp) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Operator";
    ss << " " << ModelPrinter::to_str(op);
    ss << " , First argument has an incompatible type";
    ss << *exp;
    return (ss.str());
}

inline const string TC_WRONG_SND_ARG(
        const shared_ptr<ModuleScope> &curr, ExpOp op,
        const shared_ptr<Exp> &exp) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Operator";
    ss << " " << ModelPrinter::to_str(op);
    ss << " , Second argument has an incompatible type";
    ss << *exp;
    return (ss.str());
}

inline const string TC_WRONG_PROPERTY_LEFT(shared_ptr<Prop> prop,
                                           Type last_type) {
    stringstream ss;
    ss << "Property";
    ss << ModelPrinter::to_str(prop->type);
    ss << " expression must be boolean";
    ss << *(prop->left);
    ss << " - " << UNEXPECTED_TYPE(Type::tbool, last_type);
    return (ss.str());
}

inline const string TC_WRONG_PROPERTY_RIGHT(shared_ptr<Prop> prop,
                                           Type last_type) {
    stringstream ss;
    ss << "Property";
    ss << ModelPrinter::to_str(prop->type);
    ss << " right expression must be boolean";
    ss << *(prop->right);
    ss << " - " << UNEXPECTED_TYPE(Type::tbool, last_type);
    return (ss.str());
}

inline const string TC_NOT_DNF_PROPERTY(PropType type, shared_ptr<Exp> exp) {
    stringstream ss;
    ss << "Property";
    ss << " " << ModelPrinter::to_str(type);
    ss << *exp;
    ss << " must be in Disjunctive Normal Form";
    return (ss.str());
}

inline const string TC_NOT_REDUCIBLE(shared_ptr<Exp> exp) {
    stringstream ss;
    ss << "Expression";
    ss << *exp;
    ss << " must be reducible at compilation time";
    return (ss.str());
}

inline const string TC_WRONG_INIT_VALUE(shared_ptr<Exp> exp) {
    stringstream ss;
    ss << "Initialization value incompatible with the provided range";
    ss << *exp;
    return (ss.str());
}

}

bool type_leq(Type t1, Type t2) {
    bool res = (t1 == Type::tint && t2 == Type::tfloat);
    res = res || (t1 == t2);
    return (res);
}

inline void ModelTC::check_type(Type type, const string &msg) {
    if (!has_errors() && !type_leq(last_type, type)) {
        put_error(msg);
    }
}

inline Type ModelTC::identifier_type(const string &id) {
    Type type = Type::tunknown;
    if (globals.find(id) != globals.end()) {
        type = globals[id]->type;
    }
    if (type == Type::tunknown && current_scope != nullptr) {
        auto &local = current_scope->local_decls;
        if (local.find(id) != local.end()) {
            type = local[id]->type;
        }
    }
    if (type == Type::tunknown && checking_property) {
        //when checking property, identifiers could be in any module
        shared_ptr<Decl> decl = ModuleScope::find_in_all_modules(id);
        if (decl != nullptr) {
            type = decl->type;
        }
    }
    return (type);
}

inline Type _numeric_result(Type type) {
    Type result;
    if (type == Type::tint) {
        result = type;
    } else if (type == Type::tfloat) {
        result = type;
    } else {
        result = Type::tunknown;
    }
    return (result);
}

inline Type _bool_op(Type type) {
    Type result;
    if (type == Type::tbool) {
        result = type;
    } else {
        result = Type::tunknown;
    }
    return (result);
}

inline Type _rel_op(Type type) {
    Type result;
    if (type == Type::tint || type == Type::tfloat) {
        result = Type::tbool;
    } else {
        result = Type::tunknown;
    }
    return (result);
}

inline Type ModelTC::operator_type(const ExpOp &op, Type arg) {
    Type result = Type::tunknown;
    switch(op) {
    case ExpOp::plus: result = _numeric_result(arg); break;
    case ExpOp::times: result = _numeric_result(arg); break;
    case ExpOp::minus: result = _numeric_result(arg); break;
    case ExpOp::div: result = _numeric_result(arg); break;
    case ExpOp::mod: result = _numeric_result(arg); break;
    case ExpOp::andd: result = _bool_op(arg); break;
    case ExpOp::orr: result = _bool_op(arg); break;
    case ExpOp::nott: result = _bool_op(arg); break;
    case ExpOp::eq: result = Type::tbool; break; //equality for all types?
    case ExpOp::neq: result = Type::tbool;  break;
    case ExpOp::lt: result = _rel_op(arg); break;
    case ExpOp::gt: result = _rel_op(arg); break;
    case ExpOp::le: result = _rel_op(arg); break;
    case ExpOp::ge: result = _rel_op(arg); break;
    }
    return (result);
}

inline void ModelTC::accept_cond(shared_ptr<ModelAST> node) {
    if (!has_errors()) {
        node->accept(*this);
    }
}

void ModelTC::check_clocks(shared_ptr<ModuleScope> scope) {
    for (auto entry : scope->local_decls) {
        shared_ptr<Decl> decl = entry.second;
        const string &id = entry.first;
        if (decl->type == Type::tclock) {
            if (scope->clock_dists.find(id) == scope->clock_dists.end()) {
                put_error(TC_MISSING_CLOCK_DIST(scope, id, decl));
            }
        }
    }
}

int ModelTC::eval_int_or_put(shared_ptr<Exp> exp) {
    ExpEvaluator ev;
    exp->accept(ev);
    int result = -1;
    if (ev.has_errors()) {
        put_error(::TC_NOT_REDUCIBLE(exp));
    } else {
        result = ev.get_int();
    }
    return (result);
}

float ModelTC::eval_float_or_put(shared_ptr<Exp> exp) {
    ExpEvaluator ev;
    exp->accept(ev);
    float result = -1;
    if (ev.has_errors()) {
        put_error(::TC_NOT_REDUCIBLE(exp));
    } else {
        result = ev.get_float();
    }
    return (result);
}

void ModelTC::check_decl(shared_ptr<Decl> decl) {
    if (decl->has_range()) {
        int low  = eval_int_or_put(decl->lower);
        int upp  = eval_int_or_put(decl->upper);
        if (!has_errors() && decl->has_single_init()) {
            int init = eval_int_or_put(decl->inits.at(0));
            if (! (low <= init && init <= upp)) {
                put_error(::TC_WRONG_INIT_VALUE(decl->inits.at(0)));
            }
        }
    }
}

void ModelTC::check_dist(shared_ptr<Dist> dist) {
    if (dist->arity == Arity::one) {
        eval_float_or_put(dist->param1);
    } else if (dist->arity == Arity::two) {
        eval_float_or_put(dist->param1);
        eval_float_or_put(dist->param2);
    }
}

void ModelTC::check_dist(shared_ptr<ModuleScope> scope) {
    for (auto entry : scope->clock_dists) {
        check_dist(entry.second);
    }
}

void ModelTC::check_decl_all(shared_ptr<ModuleScope> scope) {
    for (auto entry : scope->local_decls) {
        check_decl(entry.second);
    }
}

void ModelTC::visit(shared_ptr<Model> model) {
    //check globals
    for (auto decl : model->get_globals()) {
        //no module selected yet
        current_scope = nullptr;
        accept_cond(decl);
    }
    //check modules
    auto& bodies = model->get_modules();
    auto& ids = model->get_modules_ids();
    unsigned int i = 0;
    while (i < bodies.size()) {
        const shared_ptr<ModuleScope>& new_scope = make_shared<ModuleScope>();
        const string &id = ids[i];
        if (scopes.find(id) != scopes.end()) {
            shared_ptr<ModelAST> prev = scopes[id]->body;
            put_error(TC_ID_REDEFINED(current_scope, prev, id));
        }
        new_scope->body = bodies[i];
        new_scope->id = id;
        //set current scope before accepting module body
        current_scope = new_scope;
        scopes[id] = new_scope;
        accept_cond(new_scope->body);
        i++;
    }
    current_scope = nullptr;
    for (auto prop : model->get_props()) {
        accept_cond(prop);
    }
    //some extra checks after "scopes" has been built
    if (!has_errors()) {
        for (auto entry : ModuleScope::globals) {
            check_decl(entry.second);
        }
        for (auto entry : scopes) {
            const auto &scope_current = entry.second;
            check_clocks(scope_current);
            check_decl_all(scope_current);
            check_dist(scope_current);
        }
    }
}

void ModelTC::visit(shared_ptr<ModuleBody> body) {
    assert(body != nullptr);
    assert(current_scope != nullptr);
    assert(current_scope->body == body);
    for (auto &decl : body->get_local_decls()) {
        accept_cond(decl);
    }
    for (auto &action : body->get_actions()) {
        accept_cond(action);
    }
}

void ModelTC::visit(shared_ptr<Decl> decl) {
    assert(decl != nullptr);
    const string &id = decl->id;
    //check expressions before adding identifier to scope.
    //e.g. this should be ill typed: x : bool init x;
    if (decl->has_range()) {
        //check range expressions
        accept_cond(decl->lower);
        auto &error = TC_WRONG_LOWER_BOUND;
        check_type(Type::tint,
                   error(current_scope, id, decl->lower, last_type));
        accept_cond(decl->upper);
        auto &error2 = TC_WRONG_UPPER_BOUND;
        check_type(Type::tint,
                   error2(current_scope, id, decl->upper, last_type));
    }
    if (decl->is_array()) {
        //check array size expression
        accept_cond(decl->size);
        check_type(Type::tint,
                   TC_WRONG_SIZE_EXP(current_scope, id, decl->size, last_type));
    }
    for (auto &init : decl->get_inits()) {
        //check initialization expressions
        accept_cond(init);
        check_type(decl->type,
                   TC_WRONG_INIT_EXP(current_scope, id, init,
                                     decl->type, last_type));
    }
    if (is_global_scope()) {
        //check if already in global scope
        if (globals.find(id) != globals.end()) {
            shared_ptr<ModelAST> prev = globals[id];
            put_error(TC_ID_REDEFINED(current_scope, prev, id));
        } else {
            globals[id] = decl;
        }
    } else {
        //check if decl is already in local scope
        auto &local = current_scope->local_decls;
        if (local.find(id) != local.end()) {
            shared_ptr<ModelAST> prev = local[id];
            put_error(TC_ID_REDEFINED(current_scope, prev, id));
        } else {
            local[id] = decl;
        }
    }
}

void ModelTC::visit(shared_ptr<Action> action) {
    assert(action != nullptr);
    assert(current_scope != nullptr);
    string &label = action->id;
    LabelType &label_type = action->type;
    current_scope->label_actions.insert(std::make_pair(label, action));
    auto &labels = current_scope->labels;
    if (label_type != LabelType::empty) {
        //check if label is already used with another type
        if (labels.find(label) != labels.end()) {
            LabelType &other = labels[label];
            if (label_type != other) {
                put_error(TC_MULTIPLE_LABEL_TYPE(current_scope, label));
            }
        } else {
            labels[label] = label_type;
        }
    }
    //Note: output label has clock: ensured by grammar.
    //Note: input label has no clock: ensured by grammar.
    assert(action->guard != nullptr);
    accept_cond(action->guard);
    check_type(Type::tbool,
               TC_WRONG_PRECONDITION(current_scope, action, last_type)) ;
    if (action->has_clock()) {
        accept_cond(action->clock_loc);
        const string &clock_id = action->clock_loc->id;
        check_type(Type::tclock,
                   TC_NOT_A_CLOCK(current_scope, action, last_type));
        current_scope->triggered_actions.
                insert(std::make_pair(clock_id, action));
    }
    for (auto &effect : action->get_effects()) {
        accept_cond(effect);
    }
}

void ModelTC::visit(shared_ptr<Effect> effect) {
    assert(effect != nullptr);
    accept_cond(effect->loc);
    //save type of location
    Type loc_type = last_type;
    if (effect->is_clock_reset()) {
        accept_cond(effect->dist);
        // check that the same clock is not reseted
        // with diferent **type** of distribution
        // parameters should be checked after constants
        // reduction
        auto &clock_dists = current_scope->clock_dists;
        string &clock_id = effect->loc->id;
        if (clock_dists.find(clock_id) != clock_dists.end()) {
            shared_ptr<Dist> dist = clock_dists[clock_id];
            DistType dist_type = dist->type;
            if (dist_type != effect->dist->type) {
                put_error(TC_MULTIPLE_CLOCK_DIST(current_scope,
                                                 dist, effect->dist, clock_id));
            }
        } else {
            clock_dists[clock_id] = effect->dist;
        }
    }
    if (effect->is_state_change()) {
        accept_cond(effect->arg);
        check_type(loc_type,
                   TC_WRONG_RHS(current_scope, effect, loc_type, last_type));
    }
}

void ModelTC::visit(shared_ptr<Dist> dist) {
    assert(dist != nullptr);
    if (dist->arity == Arity::one) {
        accept_cond(dist->param1);
        check_type(Type::tfloat,
                   TC_WRONG_DIST_FST_PARAM(current_scope, dist, last_type));
    }
    if (dist->arity == Arity::two) {
        accept_cond(dist->param1);
        check_type(Type::tfloat,
                   TC_WRONG_DIST_FST_PARAM(current_scope, dist, last_type));
        accept_cond(dist->param2);
        check_type(Type::tfloat,
                   TC_WRONG_DIST_SND_PARAM(current_scope, dist, last_type));
    }
}

void ModelTC::visit(shared_ptr<Location> loc) {
    assert(loc != nullptr);
    string &id = loc->id;
    if (is_global_scope()) {
        //check global scope
        if (globals.find(id) == globals.end()) {
            shared_ptr<Decl> decl = nullptr;
            if (checking_property) {
                //properties can have variables declared on any module
                decl = ModuleScope::find_in_all_modules(id);
            }
            if (decl == nullptr) {
                put_error(TC_ID_OUT_OF_SCOPE(current_scope, loc));
            }
        }
    }
    else {
        auto &local = current_scope->local_decls;
        //location could be local or global
        if (local.find(id) == local.end() &&
                globals.find(id) == globals.end()) {
            put_error(TC_ID_OUT_OF_SCOPE(current_scope, loc));
        }
    }
    if (loc->is_array_position()) {
        accept_cond(loc->index);
        auto &error = TC_WRONG_INDEX_INT;
        check_type(Type::tint, error(current_scope, loc, loc->index, last_type));
    }
    last_type = identifier_type(loc->id);
}

void ModelTC::visit(shared_ptr<IConst> exp) {
    last_type = Type::tint;
    //expression should set the inferred type for itself.
    exp->type = last_type;
}

void ModelTC::visit(shared_ptr<BConst> exp){
    last_type = Type::tbool;
    exp->type = last_type;
}

void ModelTC::visit(shared_ptr<FConst> exp){
    last_type = Type::tfloat;
    exp->type = last_type;
}

void ModelTC::visit(shared_ptr<LocExp> exp){
    assert(exp != nullptr);
    accept_cond(exp->location);
    exp->type = last_type;
}

void ModelTC::visit(shared_ptr<OpExp> exp){
    assert(exp != nullptr);
    Type res_type = Type::tunknown;
    if (exp->arity == Arity::one) {
        accept_cond(exp->left);
        res_type = operator_type(exp->bop, last_type);
        if (!has_errors() && res_type == Type::tunknown) {
            put_error(TC_WRONG_FST_ARG(current_scope, exp->bop, exp));
        }
        last_type = res_type;
        exp->type = res_type;
    } else if (exp->arity == Arity::two) {
        accept_cond(exp->left);
        res_type = operator_type(exp->bop, last_type);
        if (!has_errors() && res_type == Type::tunknown) {
            put_error(TC_WRONG_FST_ARG(current_scope, exp->bop, exp));
        }
        Type fst_type = last_type;
        accept_cond(exp->right);
        res_type = operator_type(exp->bop, last_type);
        if (!has_errors() &&  res_type == Type::tunknown) {
            put_error(TC_WRONG_SND_ARG(current_scope, exp->bop, exp));
        }
        Type snd_type = last_type;
        if (! (type_leq(fst_type, snd_type) || type_leq(snd_type, fst_type))) {
            //both types should be equal or subtypes (int->float)
            put_error(TC_WRONG_SND_ARG(current_scope, exp->bop, exp));
        }
        last_type = res_type;
        exp->type = res_type;
    }
}

void ModelTC::check_dnf(PropType type, shared_ptr<Exp> exp) {
    DNFChecker dnf_checker;
    if (!has_errors()) {
        exp->accept(dnf_checker);
        if (!dnf_checker.is_dnf()) {
            put_error(TC_NOT_DNF_PROPERTY(type, exp));
        }
    }
}

void ModelTC::visit(shared_ptr<Prop> prop) {
    checking_property = true;
    accept_cond(prop->left);
    check_type(Type::tbool, TC_WRONG_PROPERTY_LEFT(prop, last_type));
    check_dnf(prop->type, prop->left);
    if (prop->type == PropType::transient) {
        accept_cond(prop->right);
        check_type(Type::tbool, TC_WRONG_PROPERTY_RIGHT(prop, last_type));
        check_dnf(prop->type, prop->right);
    }
    checking_property = false;
}

ModelTC::~ModelTC() {
}
