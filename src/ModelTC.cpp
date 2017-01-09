/* Leonardo Rodríguez */

#include <cassert>
#include <sstream>
#include <functional>

#include "ModelTC.h"
#include "ModelPrinter.h"
#include "DNFChecker.h"
#include "location.hh"
#include "ExpEvaluator.h"
#include "Operators.h"
#include "Util.h"
#include "ErrorMessage.h"

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
    if (model.get_file_location() != nullptr) {
        ss << " [at " << *(model.get_file_location()) << "]";
    }
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
    ss << " - Identifier ";
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
    ss << " - Identifier ";
    ss << " \"" << id << "\"";
    ss << " is already declared ";
    ss << *prev;
    return (ss.str());
}

inline const string TC_ID_OUT_OF_SCOPE(const shared_ptr<ModuleScope> &curr,
                                       const shared_ptr<Location> &loc) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Identifier ";
    ss << " \"" << loc->get_identifier() << "\" ";
    ss << *loc;
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
    ss << " - Identifier ";
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
    ss << " - Identifier ";
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
    ss << " - Identifier ";
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
    ss << " - Identifier ";
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

inline void show_binary_types(stringstream &ss, ExpOp op) {
    for (auto ty: Operator::binary_types(op)) {
        ss << "(" << ModelPrinter::to_str(op) << "): "
           << ty.to_string() << std::endl;
    }
}

inline void show_unary_types(stringstream &ss, ExpOp op) {
    for (auto ty: Operator::unary_types(op)) {
        ss << "(" << ModelPrinter::to_str(op) << "): "
           << ty.to_string() << std::endl;
    }
}

inline const string TC_WRONG_UNOP(
        const shared_ptr<ModuleScope> &curr, ExpOp op,
        const shared_ptr<UnOpExp> &exp, Type expected_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Wrong type for argument of operator";
    ss << " \"" << ModelPrinter::to_str(op) << "\"" << std::endl;
    ss << *exp;
    ss << std::endl;
    ss << "- Argument has type: "
       << ModelPrinter::to_str(exp->get_argument()->get_type())
       << std::endl;
    ss << "- Expected type is: "
       << ModelPrinter::to_str(expected_type)
       << std::endl;
    ss << "- Candidates are: " << std::endl;
    show_unary_types(ss, op);
    return (ss.str());
}

inline const string TC_WRONG_BINOP(
        const shared_ptr<ModuleScope> &curr, ExpOp op,
        const shared_ptr<BinOpExp> &exp, Type expected_type) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Wrong type for arguments of operator";
    ss << " \"" << ModelPrinter::to_str(op) << "\"" << std::endl;
    ss << *exp;
    ss << std::endl;
    ss << "- Argument 1 has type: "
       << ModelPrinter::to_str(exp->get_first_argument()->get_type())
       << std::endl;
    ss << "- Argument 2 has type: "
       << ModelPrinter::to_str(exp->get_second_argument()->get_type())
       << std::endl;
    ss << "- Expected type is: " << ModelPrinter::to_str(expected_type)
       << std::endl;
    ss << "- Candidates are: " << std::endl;
    show_binary_types(ss, op);
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

inline const string TC_NOT_ARRAY(const shared_ptr<ModuleScope> &curr,
                                 const shared_ptr<Location> &loc) {
    stringstream ss;
    ss << PREFIX(curr);
    ss << " - Identifier ";
    ss << " \"" << loc->get_identifier() << "\" ";
    ss << *loc;
    ss << " is not an array";
    return (ss.str());
}

} //namespace

inline void ModelTC::check_type(Type type, const string &msg) {
    if (!has_errors() && !(BasicTy(last_type) <= BasicTy(type))) {
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

inline void ModelTC::accept_cond(shared_ptr<ModelAST> node) {
    if (!has_errors()) {
        node->accept(*this);
    }
}

inline void ModelTC::accept_exp(Type expected, shared_ptr<Exp> exp) {
    expected_exp_type = expected;
    accept_cond(exp);
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
        put_error(ev.get_messages() + ::TC_NOT_REDUCIBLE(exp));
    } else {
        result = ev.get_float();
    }
    return (result);
}

void ModelTC::check_ranges(shared_ptr<Decl> decl) {
    if (decl->has_range()) {
        shared_ptr<Ranged> ranged = decl->to_ranged();
        int low  = eval_int_or_put(ranged->get_lower_bound());
        int upp  = eval_int_or_put(ranged->get_upper_bound());
        if (!has_errors() && decl->has_init()) {
            shared_ptr<Initialized> in = decl->to_initialized();
            int init = eval_int_or_put(in->get_init());
            if (! (low <= init && init <= upp)) {
                put_error(::TC_WRONG_INIT_VALUE(in->get_init()));
            }
        }
    }
}

void ModelTC::check_ranged_all(shared_ptr<ModuleScope> scope) {
    for (auto entry : scope->local_decls_map()) {
        shared_ptr<Decl> decl = entry.second;
        if (decl->has_range()) {
            check_ranges(decl);
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
                check_ranges(decl);
            }
        }
        for (auto entry : scopes) {
            const auto &scope_current = entry.second;
            current_scope = scope_current;
            check_clocks(scope_current);
            check_ranged_all(scope_current);
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
    assert(decl != nullptr);
    const string &id = decl->get_id();
    shared_ptr<Exp> lower = decl->get_lower_bound();
    shared_ptr<Exp> upper = decl->get_upper_bound();
    shared_ptr<Exp> init = decl->get_init();
    Type decl_type = decl->get_type();
    assert(decl_type == Type::tint);
    accept_exp(decl_type, lower);
    const string msg
            = TC_WRONG_LOWER_BOUND(current_scope, id, lower, last_type);
    check_type(decl_type, msg);
    accept_exp(decl_type, upper);
    const string msg2
            = TC_WRONG_UPPER_BOUND(current_scope, id, upper, last_type);
    check_type(decl_type, msg2);
    accept_exp(decl_type, init);
    const string msg3
            = TC_WRONG_INIT_EXP(current_scope, id, init, decl_type, last_type);
    check_type(decl_type, msg3);
    //@note expressions of the range are checked before adding the declared
    //identifier into scope, to forbid circular declarations
    //like "q : [0...q+1];"
    check_scope(decl);
}

void ModelTC::visit(shared_ptr<InitializedDecl> decl) {
    assert(decl != nullptr);
    shared_ptr<Exp> init = decl->get_init();
    Type decl_type = decl->get_type();
    const string &id = decl->get_id();
    accept_exp(decl_type, init);
    const string msg
            = TC_WRONG_INIT_EXP(current_scope, id, init, decl_type, last_type);
    check_type(decl_type, msg);
    check_scope(decl);
}

void ModelTC::check_array_size(shared_ptr<ArrayDecl> decl) {
    //check array size expression
    const string &id = decl->get_id();
    shared_ptr<Exp> size_exp = decl->get_size();
    accept_exp(Type::tint, size_exp);
    const string &msg
            = TC_WRONG_SIZE_EXP(current_scope, id, size_exp, last_type);
    check_type(Type::tint, msg);
}

void ModelTC::check_array_range(const std::string& id,
                                shared_ptr<Ranged> decl) {
    shared_ptr<Exp> lower = decl->get_lower_bound();
    shared_ptr<Exp> upper = decl->get_upper_bound();
    Type decl_type = Type::tint; //ranged only allowed to be integers
    accept_exp(decl_type, lower);
    const string msg2
            = TC_WRONG_LOWER_BOUND(current_scope, id, lower, last_type);
    check_type(decl_type, msg2);
    accept_exp(decl_type, upper);
    const string msg3
            = TC_WRONG_UPPER_BOUND(current_scope, id, upper, last_type);
    check_type(decl_type, msg3);
}

void ModelTC::check_array_init(const std::string &id, Type decl_type,
                               shared_ptr<Initialized> decl) {
    shared_ptr<Exp> init = decl->get_init();
    accept_exp(decl_type, init);
    const string msg4
            = TC_WRONG_INIT_EXP(current_scope, id, init, decl_type, last_type);
    check_type(decl_type, msg4);
}

void ModelTC::check_array_multiple_init(const std::string &id, Type decl_type,
                                        shared_ptr<MultipleInitialized> decl) {
    const std::vector<shared_ptr<Exp>> inits = decl->get_inits();
    for (shared_ptr<Exp> init : inits) {
        accept_exp(decl_type, init);
        const string msg4
                = TC_WRONG_INIT_EXP(current_scope, id,
                                    init, decl_type, last_type);
        check_type(decl_type, msg4);
    }
}

void ModelTC::visit(shared_ptr<RangedInitializedArray> decl) {
    const string &id = decl->get_id();
    check_array_size(decl);
    check_array_range(id, decl);
    check_array_init(id, Type::tint, decl);
    check_scope(decl);
}

void ModelTC::visit(shared_ptr<RangedMultipleInitializedArray> decl) {
    const string &id = decl->get_id();
    check_array_size(decl);
    check_array_range(id, decl);
    check_array_multiple_init(id, Type::tint, decl);
    check_scope(decl);
}

void ModelTC::visit(shared_ptr<InitializedArray> decl) {
    const string &id = decl->get_id();
    check_array_size(decl);
    if (decl->get_type() != Type::tboolarray) {
        put_error("Only boolean arrays are supported "
                  "when range is not declared.");
        return;
    }
    check_array_init(id, Type::tbool, decl);
    check_scope(decl);
}

void ModelTC::visit(shared_ptr<MultipleInitializedArray> decl) {
    const string &id = decl->get_id();
    check_array_size(decl);
    if (decl->get_type() != Type::tboolarray) {
        put_error("Only boolean arrays are supported "
                  "when range is not declared.");
        return;
    }
    check_array_multiple_init(id, Type::tbool, decl);
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
    accept_exp(Type::tbool, action->get_precondition());
    const string &msg
            = TC_WRONG_PRECONDITION(current_scope, action, last_type);
    check_type(Type::tbool, msg) ;
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
    accept_exp(loc_type, effect->get_rhs());
    const string &msg =
            TC_WRONG_RHS(current_scope, effect, loc_type, last_type);
    check_type(loc_type, msg);
}

void ModelTC::visit(shared_ptr<MultipleParameterDist> dist) {
    assert(dist != nullptr);
    accept_exp(Type::tfloat, dist->get_first_parameter());
    check_type(Type::tfloat,
               TC_WRONG_DIST_FST_PARAM(current_scope, dist, last_type));
    accept_exp(Type::tfloat, dist->get_second_parameter());
    check_type(Type::tfloat,
               TC_WRONG_DIST_SND_PARAM(current_scope, dist, last_type));
}

void ModelTC::visit(shared_ptr<SingleParameterDist> dist) {
    accept_exp(Type::tfloat, dist->get_parameter());
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
    accept_exp(Type::tint, loc->get_index());
    const string &msg =
            TC_WRONG_INDEX_INT(current_scope, loc, loc->get_index(), last_type);
    check_type(Type::tint, msg);
    visit(std::static_pointer_cast<Location>(loc));
    last_type = Ty::array_elem_type(last_type);
    if (last_type == Type::tunknown) {
        put_error(::TC_NOT_ARRAY(current_scope, loc));
    }
}

void ModelTC::visit(shared_ptr<IConst> exp) {
    assert(expected_exp_type != Type::tunknown);
    last_type = Type::tint;
    //expression should set the inferred type for itself.
    exp->set_type(last_type);
}

void ModelTC::visit(shared_ptr<BConst> exp){
    assert(expected_exp_type != Type::tunknown);
    last_type = Type::tbool;
    exp->set_type(last_type);
}

void ModelTC::visit(shared_ptr<FConst> exp){
    assert(expected_exp_type != Type::tunknown);
    last_type = Type::tfloat;
    exp->set_type(last_type);
}

void ModelTC::visit(shared_ptr<LocExp> exp){
    assert(expected_exp_type != Type::tunknown);
    assert(exp != nullptr);
    accept_cond(exp->get_exp_location());
    exp->set_type(last_type);
}

// Check an expressions on its own instance of ModelTC
bool ModelTC::dummy_check(Type expected_type, shared_ptr<Exp> exp) {
    ModelTC dummy (*this);
    dummy.message = make_shared<ErrorMessage>();
    dummy.expected_exp_type = expected_type;
    exp->accept(dummy);
    //copy result to this typechecker
    last_type = dummy.last_type;
    return (!dummy.has_errors());
}

void ModelTC::visit(shared_ptr<BinOpExp> exp) {
    assert(expected_exp_type != Type::tunknown);
    assert(exp != nullptr);
    Type type_expected = expected_exp_type;
    ExpOp op = exp->get_operator();
    vector<BinaryOpTy> types = Operator::binary_types(op);
    auto ty_gt = [] (const BinaryOpTy& ty1, const BinaryOpTy& ty2) {
        return (ty2 < ty1);
    };
    //sort the type candidates for the operator
    std::sort(types.begin(), types.end(), ty_gt);
    //find a compatible type
    shared_ptr<Exp> arg1 = exp->get_first_argument();
    shared_ptr<Exp> arg2 = exp->get_second_argument();
    shared_ptr<BinaryOpTy> selected = nullptr;
    unsigned int i = 0;
    while (i < types.size() && selected == nullptr) {
        BinaryOpTy ty = types[i];
        bool ok1 = dummy_check(ty.get_arg1_type(), arg1);
        Type inferred1 = last_type;
        bool ok2 = dummy_check(ty.get_arg2_type(), arg2);
        Type inferred2 = last_type;
        BinaryOpTy inf (inferred1, inferred2, type_expected);
        if (ty <= inf && ok1 && ok2) {
            selected = make_shared<BinaryOpTy>(ty);
        }
        i++;
    }
    if (selected != nullptr) {
        Type res_type = selected->get_result_type();
        last_type = res_type;
        exp->set_inferred_type(*selected);
        exp->set_type(res_type);
    } else {
        put_error(TC_WRONG_BINOP(current_scope, op, exp, type_expected));
    }
}

void ModelTC::visit(shared_ptr<UnOpExp> exp) {
    assert(expected_exp_type != Type::tunknown);
    ExpOp op = exp->get_operator();
    Type type_expected = expected_exp_type;
    vector<UnaryOpTy> types = Operator::unary_types(op);
    auto ty_gt = [] (const UnaryOpTy &ty1, const UnaryOpTy &ty2) {
        return (ty2 < ty1);
    };
    std::sort(types.begin(), types.end(), ty_gt);
    shared_ptr<Exp> arg1 = exp->get_argument();
    shared_ptr<UnaryOpTy> selected = nullptr;
    unsigned int i = 0;
    while (i < types.size() && selected == nullptr) {
        UnaryOpTy ty = types[i];
        bool ok = dummy_check(ty.get_arg_type(), arg1);
        Type inferred1 = last_type;
        UnaryOpTy inf (inferred1, type_expected);
        if (ty <= inf && ok) {
            selected = make_shared<UnaryOpTy>(ty);
        }
        i++;
    }
    if (selected != nullptr) {
        Type res_type = selected->get_result_type();
        last_type = res_type;
        exp->set_inferred_type(*selected);
        exp->set_type(res_type);
    } else {
        put_error(TC_WRONG_UNOP(current_scope, op, exp, type_expected));
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

void ModelTC::visit(shared_ptr<TransientProp> prop) {
    checking_property = true;
    accept_exp(Type::tbool, prop->get_left());
    check_type(Type::tbool, TC_WRONG_PROPERTY_LEFT(prop, last_type));
    check_dnf(prop->get_type(), prop->get_left());
    accept_exp(Type::tbool, prop->get_right());
    check_type(Type::tbool, TC_WRONG_PROPERTY_RIGHT(prop, last_type));
    check_dnf(prop->get_type(), prop->get_right());
    checking_property = false;
}

void ModelTC::visit(shared_ptr<RateProp> prop) {
    checking_property = true;
    accept_exp(Type::tbool, prop->get_expression());
    check_type(Type::tbool, TC_WRONG_PROPERTY_EXP(prop, last_type));
    check_dnf(prop->get_type(), prop->get_expression());
    checking_property = false;
}

ModelTC::~ModelTC() {
}
