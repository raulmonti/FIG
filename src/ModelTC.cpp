/* Leonardo Rodríguez */

#include "ModelTC.h"
#include "ModelPrinter.h"
#include "DNFChecker.h"
#include <cassert>

using std::endl;
using std::shared_ptr;
using std::make_pair;

/* Typechecking on Model */

//initialize static maps
shared_map<string, ModuleScope> ModuleScope::scopes;
shared_map<string, Decl> ModuleScope::globals;

//Some error messages:
namespace {

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

inline const string TC_WRONG_INDEX_INT(const shared_ptr<ModuleScope>& curr,
                                       const string &id,
                                       Type last_type) {
    return PREFIX(curr) +
            " - Identifier \""  +
            id +
            "\" - Index expression - " +
            UNEXPECTED_TYPE(Type::tint, last_type);
}

inline const string TC_ID_REDEFINED(const shared_ptr<ModuleScope>& curr,
                                    const string &id) {
    return PREFIX(curr) +
            " - Identifier \""  +
            id +
            "\" was redefined";
}

inline const string TC_ID_OUT_OF_SCOPE(const shared_ptr<ModuleScope>& curr,
                                       const string &id) {
    return PREFIX(curr) +
            " - Identifier \""  +
            id +
            "\" is not in scope";
}

inline const string TC_WRONG_LOWER_BOUND(
        const shared_ptr<ModuleScope>& curr,
        const string &id,
        const Type last_type) {
    return PREFIX(curr) +
            " - Identifier \"" +
            id +
            "\" - Lower bound of range is ill-typed - " +
            UNEXPECTED_TYPE(Type::tint, last_type);
}

inline const string TC_WRONG_UPPER_BOUND(
        const shared_ptr<ModuleScope>& curr,
        const string &id,
        const Type last_type) {
    return PREFIX(curr) +
            " - Identifier \"" +
            id +
            "\" - Upper bound of range is ill-typed - " +
            UNEXPECTED_TYPE(Type::tint, last_type);
}

inline const string TC_WRONG_SIZE_EXP(const shared_ptr<ModuleScope>& curr,
                                      const string &id,
                                      const Type last_type) {

    return PREFIX(curr) +
            " - Identifier \""  +
            id +
            "\" - Array size expression is ill typed - " +
            UNEXPECTED_TYPE(Type::tint, last_type);
}

inline const string TC_WRONG_DIST_FST_PARAM(
        const shared_ptr<ModuleScope>& curr,
        const DistType &dist,
        const Type last_type) {

    return  PREFIX(curr) +
            " - Distribution " +
            ModelPrinter::to_str(dist) +
            " - First parameter is ill typed - " +
            UNEXPECTED_TYPE(Type::tfloat, last_type);
}

inline const string TC_WRONG_DIST_SND_PARAM(
        const shared_ptr<ModuleScope>& curr,
        const DistType &dist,
        const Type last_type) {

    return  PREFIX(curr) +
            " - Distribution " +
            ModelPrinter::to_str(dist) +
            " - Second parameter is ill typed - " +
            UNEXPECTED_TYPE(Type::tfloat, last_type);
}

inline const string TC_WRONG_INIT_EXP(const shared_ptr<ModuleScope>& curr,
                                      const string &id,
                                      const Type expected,
                                      const Type last_type) {
    return PREFIX(curr) +
            " - Identifier \""  +
            id +
            "\" - Initializer is ill-typed - " +
            UNEXPECTED_TYPE(expected, last_type);
}

inline const string TC_MULTIPLE_LABEL_TYPE(
        const shared_ptr<ModuleScope>& curr,
        const string &label) {

    return PREFIX(curr) +
            " - Label \""  +
            label +
            "\" must have a single type ";
}

inline const string TC_MULTIPLE_CLOCK_DIST(
        const shared_ptr<ModuleScope>& curr,
        const string &clock_id) {

    return PREFIX(curr) +
            " - Clock \""  +
            clock_id +
            "\" must have a single distribution type ";
}

inline const string TC_MISSING_CLOCK_DIST(
        const shared_ptr<ModuleScope>& curr,
        const string &clock_id) {

    return PREFIX(curr) +
            " - Clock \""  +
            clock_id +
            "\" must have a distribution type";
}

inline const string TC_MULTIPLE_CLOCK_FOR_LABEL(
        const shared_ptr<ModuleScope>& curr,
        const string &label) {

    return PREFIX(curr) +
            " - Label \""  +
            label +
            "\" must have a single triggering clock";
}

inline const string TC_NOT_A_CLOCK(
        const shared_ptr<ModuleScope>& curr,
        const string &label,
        const string &clock_id,
        Type last_type) {

    return PREFIX(curr) +
            " - Transition of Label \""  +
            label +
            "\" - Identifier \"" +
            clock_id +
            "\" is not a clock - " +
            UNEXPECTED_TYPE(Type::tint, last_type);
}

inline const string TC_WRONG_PRECONDITION(
        const shared_ptr<ModuleScope>& curr,
        const string &label,
        Type last_type) {

    return PREFIX(curr) +
            " - Transition of Label \""  +
            label +
            "\" - Precondition is ill-typed " +
            UNEXPECTED_TYPE(Type::tbool, last_type);
}

inline const string TC_WRONG_PRECONDITION_S(
        const shared_ptr<ModuleScope>& curr,
        Type last_type) {

    return PREFIX(curr)		+
            " - Transition of silent label - "  +
            " - Precondition is ill-typed - " +
            UNEXPECTED_TYPE(Type::tbool, last_type);
}

inline const string TC_WRONG_RHS(const shared_ptr<ModuleScope>& curr,
                                 const string &id,
                                 Type expected,
                                 Type last_type) {
    return PREFIX(curr) +
            " - Assignment of state \""  +
            id +
            "'\" - Right-hand side expression is ill-typed - "	+
            UNEXPECTED_TYPE(expected, last_type);
}

inline const string TC_WRONG_FST_ARG(
        const shared_ptr<ModuleScope>& curr, ExpOp op) {
    return PREFIX(curr) +
            " - Operator "  +
            ModelPrinter::to_str(op) +
            " - First argument has an incompatible type ";
}

inline const string TC_WRONG_SND_ARG(
        const shared_ptr<ModuleScope>& curr, ExpOp op) {
    return PREFIX(curr) +
            " - Operator "  +
            ModelPrinter::to_str(op) +
            " - Second argument has an incompatible type ";
}

inline const string TC_WRONG_PROPERTY_LEFT(PropType type, Type last_type) {
    return "Property " + ModelPrinter::to_str(type)
            + " expressions should be boolean - " +
            UNEXPECTED_TYPE(Type::tbool, last_type);
}


inline const string TC_WRONG_PROPERTY_RIGHT(PropType type, Type last_type) {
    return "Property " + ModelPrinter::to_str(type)
            + " expressions should be boolean - " +
            UNEXPECTED_TYPE(Type::tbool, last_type);
}

inline const string TC_NOT_DNF_PROPERTY(PropType type) {
    return "Property " + ModelPrinter::to_str(type)
            + " should be in Disjunctive Normal Form";
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
                put_error(TC_MISSING_CLOCK_DIST(scope, id));
            }
        }
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
            put_error(TC_ID_REDEFINED(current_scope, id));
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
        for (auto entry : scopes) {
            const auto &scope_current = entry.second;
            check_clocks(scope_current);
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
        check_type(Type::tint,
                   TC_WRONG_LOWER_BOUND(current_scope, id, last_type));
        accept_cond(decl->upper);
        check_type(Type::tint,
                   TC_WRONG_UPPER_BOUND(current_scope, id, last_type));
    }
    if (decl->is_array()) {
        //check array size expression
        accept_cond(decl->size);
        check_type(Type::tint, TC_WRONG_SIZE_EXP(current_scope, id, last_type));
    }
    for (auto &init : decl->get_inits()) {
        //check initialization expressions
        accept_cond(init);
        check_type(decl->type,
                   TC_WRONG_INIT_EXP(current_scope, id, decl->type, last_type));
    }
    if (is_global_scope()) {
        //check if already in global scope
        if (globals.find(id) != globals.end()) {
            put_error(TC_ID_REDEFINED(current_scope, id));
        } else {
            globals[id] = decl;
        }
    } else {
        //check if decl is already in local scope
        auto &local = current_scope->local_decls;
        if (local.find(id) != local.end()) {
            put_error(TC_ID_REDEFINED(current_scope, id));
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
    if (label_type != LabelType::empty) {
        check_type(Type::tbool,
                   TC_WRONG_PRECONDITION(current_scope, label, last_type)) ;
    } else {
        check_type(Type::tbool,
                   TC_WRONG_PRECONDITION_S(current_scope, last_type)) ;
    }
    if (action->has_clock()) {
        accept_cond(action->clock_loc);
        const string &clock_id = action->clock_loc->id;
        check_type(Type::tclock,
                   TC_NOT_A_CLOCK(current_scope, label, clock_id, last_type));
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
            DistType dist_type = clock_dists[clock_id]->type;
            if (dist_type != effect->dist->type) {
                put_error(TC_MULTIPLE_CLOCK_DIST(current_scope, clock_id));
            }
        } else {
            clock_dists[clock_id] = effect->dist;
        }
    }
    if (effect->is_state_change()) {
        accept_cond(effect->arg);
        check_type(loc_type,
                   TC_WRONG_RHS(current_scope,
                                effect->loc->id, loc_type, last_type));
    }
}

void ModelTC::visit(shared_ptr<Dist> dist) {
    assert(dist != nullptr);
    if (dist->arity == Arity::one) {
        accept_cond(dist->param1);
        check_type(Type::tfloat,
                   TC_WRONG_DIST_FST_PARAM(current_scope,
                                           dist->type, last_type));
    }
    if (dist->arity == Arity::two) {
        accept_cond(dist->param1);
        check_type(Type::tfloat,
                   TC_WRONG_DIST_FST_PARAM(current_scope,
                                           dist->type, last_type));
        accept_cond(dist->param2);
        check_type(Type::tfloat,
                   TC_WRONG_DIST_SND_PARAM(current_scope,
                                           dist->type, last_type));
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
                put_error(TC_ID_OUT_OF_SCOPE(current_scope, id));
            }
        }
    }
    else {
        auto &local = current_scope->local_decls;
        //location could be local or global
        if (local.find(id) == local.end() &&
                globals.find(id) == globals.end()) {
            put_error(TC_ID_OUT_OF_SCOPE(current_scope, id));
        }
    }
    if (loc->is_array_position()) {
        accept_cond(loc->index);
        check_type(Type::tint,
                   TC_WRONG_INDEX_INT(current_scope, loc->id, last_type));
    }
    last_type = identifier_type(loc->id);
}

void ModelTC::visit(shared_ptr<IConst> exp) {
    last_type = Type::tint;
    //expressions should set the inferred type for itself.
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
            put_error(TC_WRONG_FST_ARG(current_scope, exp->bop));
        }
        last_type = res_type;
        exp->type = res_type;
    } else if (exp->arity == Arity::two) {
        accept_cond(exp->left);
        res_type = operator_type(exp->bop, last_type);
        if (!has_errors() && res_type == Type::tunknown) {
            put_error(TC_WRONG_FST_ARG(current_scope, exp->bop));
        }
        Type fst_type = last_type;
        accept_cond(exp->right);
        res_type = operator_type(exp->bop, last_type);
        if (!has_errors() &&  res_type == Type::tunknown) {
            put_error(TC_WRONG_SND_ARG(current_scope, exp->bop));
        }
        Type snd_type = last_type;
        if (! (type_leq(fst_type, snd_type) || type_leq(snd_type, fst_type))) {
            //both types should be equal or subtypes (int->float)
            put_error(TC_WRONG_SND_ARG(current_scope, exp->bop));
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
            put_error(TC_NOT_DNF_PROPERTY(type));
        }
    }
}

void ModelTC::visit(shared_ptr<Prop> prop) {
    checking_property = true;
    accept_cond(prop->left);
    check_type(Type::tbool, TC_WRONG_PROPERTY_LEFT(prop->type, last_type));
    check_dnf(prop->type, prop->left);
    if (prop->type == PropType::transient) {
        accept_cond(prop->right);
        check_type(Type::tbool, TC_WRONG_PROPERTY_RIGHT(prop->type, last_type));
        check_dnf(prop->type, prop->right);
    }
    checking_property = false;
}

ModelTC::~ModelTC() {
}
