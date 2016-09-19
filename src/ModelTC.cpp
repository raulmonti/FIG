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

stringstream& operator<<(stringstream &ss, ModelAST& model) {
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
    return current_scope == nullptr
            ? "At global constants" :
              "At Module " + current_scope->get_module_name();
}

inline const string TC_WRONG_INDEX_INT(const shared_ptr<ModuleScope> &curr,
                                       const shared_ptr<Location> &loc,
                                       const shared_ptr<Exp> &exp,
                                       Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Identifier";
    ss << " \"" << loc->get_identifier() << "\"";
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
    ss << " \"" << loc->get_identifier() << "\"";
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
    ss << ModelPrinter::to_str(dist->get_type());
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
    ss << ModelPrinter::to_str(dist->get_type());
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
    ss << " - reseted as " << ModelPrinter::to_str(dist1->get_type());
    ss << *dist1;
    ss << " and reseted as " << ModelPrinter::to_str(dist2->get_type());
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
        const shared_ptr<OutputTransition> &action,
        Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Transition of label";
    ss << " \"" << action->get_label() << "\"";
    ss << *action;
    ss << " - Identifier";
    ss << " \"" << action->get_triggering_clock()->get_identifier() << "\"";
    ss << " is not a clock";
    ss << " - " << UNEXPECTED_TYPE(Type::tclock, last_type);
    return (ss.str());
}

inline const string TC_WRONG_PRECONDITION(
        const shared_ptr<ModuleScope>& curr,
        const shared_ptr<TransitionAST>& action,
        Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Transition of label";
    if (action->get_label().empty()) {
        ss << " tau";
    } else {
        ss << " \"" << action->get_label() << "\"";
    }
    ss << " - Precondition is ill-typed";
    ss << *(action->get_precondition());
    ss << " - " << UNEXPECTED_TYPE(Type::tbool, last_type);
    return (ss.str());
}

inline const string TC_WRONG_RHS(const shared_ptr<ModuleScope> &curr,
                                 const shared_ptr<Assignment> &effect,
                                 Type expected,
                                 Type last_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Assignment of state variable";
    ss << " \"" << effect->get_effect_location()->get_identifier() << "\"";
    ss << " - Right-hand side expression is ill-typed";
    ss << *(effect->get_rhs());
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

inline const string TC_WRONG_PROPERTY_LEFT(shared_ptr<TransientProp> prop,
                                           Type last_type) {
    stringstream ss;
    ss << "Property";
    ss << ModelPrinter::to_str(prop->get_type());
    ss << " expression must be boolean";
    ss << *(prop->get_left());
    ss << " - " << UNEXPECTED_TYPE(Type::tbool, last_type);
    return (ss.str());
}

inline const string TC_WRONG_PROPERTY_EXP(shared_ptr<RateProp> prop,
                                           Type last_type) {
    stringstream ss;
    ss << "Property";
    ss << ModelPrinter::to_str(prop->get_type());
    ss << " expression must be boolean";
    ss << *(prop->get_expression());
    ss << " - " << UNEXPECTED_TYPE(Type::tbool, last_type);
    return (ss.str());
}

inline const string TC_WRONG_PROPERTY_RIGHT(shared_ptr<TransientProp> prop,
                                           Type last_type) {
    stringstream ss;
    ss << "Property";
    ss << ModelPrinter::to_str(prop->get_type());
    ss << " right expression must be boolean";
    ss << *(prop->get_right());
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
        type = globals[id]->get_type();
    }
    if (type == Type::tunknown && current_scope != nullptr) {
        const auto &local = current_scope->local_decls_map();
        if (local.find(id) != local.end()) {
            type = local.at(id)->get_type();
        }
    }
    if (type == Type::tunknown && checking_property) {
        //when checking property, identifiers could be in any module
        shared_ptr<Decl> decl = ModuleScope::find_in_all_modules(id);
        if (decl != nullptr) {
            type = decl->get_type();
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
    for (auto entry : scope->local_decls_map()) {
        shared_ptr<Decl> decl = entry.second;
        const string &id = entry.first;
        if (decl->get_type() == Type::tclock) {
            if (scope->dist_by_clock_map().find(id)
                    == scope->dist_by_clock_map().end()) {
                put_error(TC_MISSING_CLOCK_DIST(scope, id, decl));
            }
        }
    }
}

int ModelTC::eval_int_or_put(shared_ptr<Exp> exp) {
    ExpEvaluator ev (current_scope);
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
    ExpEvaluator ev (current_scope);
    exp->accept(ev);
    float result = -1;
    if (ev.has_errors()) {
        put_error(::TC_NOT_REDUCIBLE(exp));
    } else {
        result = ev.get_float();
    }
    return (result);
}

void ModelTC::check_ranged_decl(shared_ptr<RangedDecl> decl) {
    int low  = eval_int_or_put(decl->get_lower_bound());
    int upp  = eval_int_or_put(decl->get_upper_bound());
    if (!has_errors()) {
        int init = eval_int_or_put(decl->get_init());
        if (! (low <= init && init <= upp)) {
            put_error(::TC_WRONG_INIT_VALUE(decl->get_init()));
        }
    }
}

void ModelTC::check_dist(shared_ptr<Dist> dist) {
    if (dist->has_single_parameter()) {
        eval_float_or_put(dist->to_single_parameter()->get_parameter());
    } else if (dist->has_multiple_parameters()) {
        eval_float_or_put(dist->to_multiple_parameter()->get_first_parameter());
        eval_float_or_put(dist->to_multiple_parameter()->get_second_parameter());
    }
}

void ModelTC::check_dist(shared_ptr<ModuleScope> scope) {
    for (auto entry : scope->dist_by_clock_map()) {
        check_dist(entry.second);
    }
}

void ModelTC::check_ranged_all(shared_ptr<ModuleScope> scope) {
    for (auto entry : scope->local_decls_map()) {
        shared_ptr<Decl> decl = entry.second;
        if (decl->has_range()) {
            check_ranged_decl(decl->to_ranged());
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
    auto& modules = model->get_modules();
    unsigned int i = 0;
    while (i < modules.size()) {
        const shared_ptr<ModuleScope>& new_scope = make_shared<ModuleScope>();
        const string &id = modules[i]->get_name();
        if (scopes.find(id) != scopes.end()) {
            shared_ptr<ModelAST> prev = scopes[id]->module_ast();
            put_error(TC_ID_REDEFINED(current_scope, prev, id));
        }
        new_scope->set_module_ast(modules[i]);
        new_scope->set_module_name(modules[i]->get_name());
        //set current scope before accepting module body
        current_scope = new_scope;
        scopes[id] = new_scope;
        accept_cond(new_scope->module_ast());
        i++;
    }
    current_scope = nullptr;
    for (auto prop : model->get_props()) {
        accept_cond(prop);
    }
    //some extra checks after "scopes" has been built
    if (!has_errors()) {
        for (auto entry : ModuleScope::globals) {
            shared_ptr<Decl> decl = entry.second;
            if (decl->has_range()) {
                check_ranged_decl(decl->to_ranged());
            }
        }
        for (auto entry : scopes) {
            const auto &scope_current = entry.second;
            current_scope = scope_current;
            check_clocks(scope_current);
            check_ranged_all(scope_current);
            check_dist(scope_current);
        }
    }
}

void ModelTC::visit(shared_ptr<ModuleAST> body) {
    assert(body != nullptr);
    assert(current_scope != nullptr);
    assert(current_scope->module_ast() == body);
    for (auto &decl : body->get_local_decls()) {
        accept_cond(decl);
    }
    for (shared_ptr<TransitionAST> action : body->get_transitions()) {
        accept_cond(action);
    }
}

void ModelTC::check_scope(shared_ptr<Decl> decl) {
    const string &id = decl->get_id();
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
        std::map<string, shared_ptr<Decl>> &local
                = current_scope->local_decls_map();
        if (local.find(id) != local.end()) {
            shared_ptr<ModelAST> prev = local.at(id);
            put_error(TC_ID_REDEFINED(current_scope, prev, id));
        } else {
            local[id] = decl;
        }
    }
}

void ModelTC::visit(shared_ptr<RangedDecl> decl) {
    const string &id = decl->get_id();
    accept_cond(decl->get_lower_bound());
    auto &error = TC_WRONG_LOWER_BOUND;
    check_type(Type::tint,
               error(current_scope, id, decl->get_lower_bound(), last_type));
    accept_cond(decl->get_upper_bound());
    auto &error2 = TC_WRONG_UPPER_BOUND;
    check_type(Type::tint,
               error2(current_scope, id, decl->get_upper_bound(), last_type));
    accept_cond(decl->get_init());
    check_type(Type::tint,
               TC_WRONG_INIT_EXP(current_scope, id, decl->get_init(),
                                 decl->get_type(), last_type));
    check_scope(decl);
}

void ModelTC::visit(shared_ptr<InitializedDecl> decl) {
    assert(decl != nullptr);
    const string &id = decl->get_id();
    accept_cond(decl->get_init());
    check_type(decl->get_type(),
               TC_WRONG_INIT_EXP(current_scope, id, decl->get_init(),
                                 decl->get_type(), last_type));
    check_scope(decl);
}

void ModelTC::visit(shared_ptr<ArrayDecl> decl) {
    //check array size expression
    const string &id = decl->get_id();
    accept_cond(decl->get_size());
    check_type(Type::tint,
               TC_WRONG_SIZE_EXP(current_scope, id,
                                 decl->get_size(), last_type));
    check_scope(decl);
}

void ModelTC::visit(shared_ptr<ClockDecl> decl) {
    check_scope(decl);
}

void ModelTC::visit(shared_ptr<TransitionAST> action) {
    assert(action != nullptr);
    assert(current_scope != nullptr);
    const string &label = action->get_label();
    const LabelType &label_type = action->get_label_type();
    current_scope->transition_by_label_map()
            .insert(std::make_pair(label, action));
    auto &labels = current_scope->type_by_label_map();
    if (label_type != LabelType::tau) {
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
    assert(action->get_precondition() != nullptr);
    accept_cond(action->get_precondition());
    check_type(Type::tbool,
               TC_WRONG_PRECONDITION(current_scope, action, last_type)) ;
    if (action->has_triggering_clock()) {
        shared_ptr<OutputTransition> output = action->to_output();
        accept_cond(output->get_triggering_clock());
        const string &clock_id
                = output->get_triggering_clock()->get_identifier();
        check_type(Type::tclock,
                   TC_NOT_A_CLOCK(current_scope, output, last_type));
        current_scope->transition_by_clock_map().
                insert(std::make_pair(clock_id, output));
    }
    for (auto &effect : action->get_assignments()) {
        accept_cond(effect);
    }
    for (auto &effect : action->get_clock_resets()) {
        accept_cond(effect);
    }
}

void ModelTC::visit(shared_ptr<ClockReset> effect) {
    accept_cond(effect->get_effect_location());
    accept_cond(effect->get_dist());
    // check that the same clock is not reseted
    // with diferent **type** of distribution
    // parameters should be checked after constants
    // reduction
    auto &clock_dists = current_scope->dist_by_clock_map();
    const string &clock_id = effect->get_effect_location()->get_identifier();
    if (clock_dists.find(clock_id) != clock_dists.end()) {
        shared_ptr<Dist> dist = clock_dists[clock_id];
        DistType dist_type = dist->get_type();
        if (dist_type != effect->get_dist()->get_type()) {
            put_error(TC_MULTIPLE_CLOCK_DIST(current_scope,
                                             dist, effect->get_dist(), clock_id));
        }
    } else {
        clock_dists[clock_id] = effect->get_dist();
    }
}

void ModelTC::visit(shared_ptr<Assignment> effect) {
    assert(effect != nullptr);
    accept_cond(effect->get_effect_location());
    Type loc_type = last_type;
    accept_cond(effect->get_rhs());
    check_type(loc_type,
               TC_WRONG_RHS(current_scope, effect, loc_type, last_type));
}

void ModelTC::visit(shared_ptr<MultipleParameterDist> dist) {
    assert(dist != nullptr);
    accept_cond(dist->get_first_parameter());
    check_type(Type::tfloat,
               TC_WRONG_DIST_FST_PARAM(current_scope, dist, last_type));
    accept_cond(dist->get_second_parameter());
    check_type(Type::tfloat,
               TC_WRONG_DIST_SND_PARAM(current_scope, dist, last_type));
}

void ModelTC::visit(shared_ptr<SingleParameterDist> dist) {
    accept_cond(dist->get_parameter());
    check_type(Type::tfloat,
               TC_WRONG_DIST_FST_PARAM(current_scope, dist, last_type));
}

void ModelTC::visit(shared_ptr<Location> loc) {
    assert(loc != nullptr);
    const string &id = loc->get_identifier();
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
    } else {
        auto &local = current_scope->local_decls_map();
        //location could be local or global
        if (local.find(id) == local.end() &&
                globals.find(id) == globals.end()) {
            put_error(TC_ID_OUT_OF_SCOPE(current_scope, loc));
        }
    }
    last_type = identifier_type(loc->get_identifier());
}

void ModelTC::visit(shared_ptr<ArrayPosition> loc) {
    accept_cond(loc->get_index());
    auto &error = TC_WRONG_INDEX_INT;
    check_type(Type::tint,
               error(current_scope, loc, loc->get_index(), last_type));
    visit(std::static_pointer_cast<Location>(loc));
}

void ModelTC::visit(shared_ptr<IConst> exp) {
    last_type = Type::tint;
    //expression should set the inferred type for itself.
    exp->set_type(last_type);
}

void ModelTC::visit(shared_ptr<BConst> exp){
    last_type = Type::tbool;
    exp->set_type(last_type);
}

void ModelTC::visit(shared_ptr<FConst> exp){
    last_type = Type::tfloat;
    exp->set_type(last_type);
}

void ModelTC::visit(shared_ptr<LocExp> exp){
    assert(exp != nullptr);
    accept_cond(exp->get_exp_location());
    exp->set_type(last_type);
}

void ModelTC::visit(shared_ptr<BinOpExp> exp){
    assert(exp != nullptr);
    Type res_type = Type::tunknown;
    accept_cond(exp->get_first_argument());
    res_type = operator_type(exp->get_operator(), last_type);
    if (!has_errors() && res_type == Type::tunknown) {
        put_error(TC_WRONG_FST_ARG(current_scope, exp->get_operator(), exp));
    }
    Type fst_type = last_type;
    accept_cond(exp->get_second_argument());
    res_type = operator_type(exp->get_operator(), last_type);
    if (!has_errors() &&  res_type == Type::tunknown) {
        put_error(TC_WRONG_SND_ARG(current_scope, exp->get_operator(), exp));
    }
    Type snd_type = last_type;
    if (! (type_leq(fst_type, snd_type) || type_leq(snd_type, fst_type))) {
        //both types should be equal or subtypes (int->float)
        put_error(TC_WRONG_SND_ARG(current_scope, exp->get_operator(), exp));
    }
    last_type = res_type;
    exp->set_type(res_type);
}

void ModelTC::visit(shared_ptr<UnOpExp> exp) {
    accept_cond(exp->get_argument());
    Type res_type = operator_type(exp->get_operator(), last_type);
    if (!has_errors() && res_type == Type::tunknown) {
        put_error(TC_WRONG_FST_ARG(current_scope, exp->get_operator(), exp));
    }
    last_type = res_type;
    exp->set_type(res_type);
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

void ModelTC::visit(shared_ptr<TransientProp> prop) {
    checking_property = true;
    accept_cond(prop->get_left());
    check_type(Type::tbool, TC_WRONG_PROPERTY_LEFT(prop, last_type));
    check_dnf(prop->get_type(), prop->get_left());
    accept_cond(prop->get_right());
    check_type(Type::tbool, TC_WRONG_PROPERTY_RIGHT(prop, last_type));
    check_dnf(prop->get_type(), prop->get_right());
    checking_property = false;
}

void ModelTC::visit(shared_ptr<RateProp> prop) {
    checking_property = true;
    accept_cond(prop->get_expression());
    check_type(Type::tbool, TC_WRONG_PROPERTY_EXP(prop, last_type));
    check_dnf(prop->get_type(), prop->get_expression());
    checking_property = false;
}

ModelTC::~ModelTC() {
}
