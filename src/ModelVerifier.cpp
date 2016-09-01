#include "ModelVerifier.h"
#include "ExpReductor.h"
#include "FigException.h"

namespace {
//"operator%" not defined in z3++.h, let's improvise one.
z3::expr z3mod(z3::expr const & a, z3::expr const & b) {
    check_context(a, b);
    Z3_ast r = 0;
    if (a.is_arith() && b.is_arith()) {
        r = Z3_mk_mod(a.ctx(), a, b);
    }
    else if (a.is_bv() && b.is_bv()) {
        r = Z3_mk_bvsmod(a.ctx(), a, b);
    }
    else {
        // operator is not supported by given arguments.
        assert(false);
    }
    a.check_error();
    return z3::expr(a.ctx(), r);
}

shared_ptr<Exp> eval_or_throw(shared_ptr<Exp> exp) {
    ExpEvaluator ev;
    exp->accept(ev);
    if (ev.has_errors()) {
        throw_FigException("Could not evaluate expression");
    }
    return (ev.value_to_ast());
}

shared_ptr<Exp> find_assignment_rhs(shared_ptr<Action> action,
                                   const string &state_id) {
    auto same_id = [state_id] (const shared_ptr<Effect> &effect) -> bool {
        return (effect->is_state_change() && effect->loc->id == state_id);
    };
    auto begin = action->effects.begin();
    auto end = action->effects.end();
    auto res = std::find_if(begin, end, same_id);
    return (res == end ? nullptr : (*res)->arg);
}

//do this action resets the given clock?
bool resets_clock(shared_ptr<Action> action, const string &clock_id) {
    auto same_clock =
            [clock_id] (const shared_ptr<Effect> &effect) -> bool {
        return (effect->is_clock_reset() && effect->loc->id == clock_id);
    };
    auto end = action->effects.end();
    auto res = std::find_if(action->effects.begin(), end, same_clock);
    return (res != end);
}

// check if *a1* reseted clocks are also reseted by *a2*
bool resets_clocks_of(shared_ptr<Action> a1, shared_ptr<Action> a2) {
    auto it1 = a1->effects.begin();
    bool result = true;
    while (it1 != a1->effects.end() && result) {
        shared_ptr<Effect> current = *it1;
        if (current->is_clock_reset()) {
            const string &clock_id = current->loc->id;
            result = result && resets_clock(a2, clock_id);
        };
        it1++;
    }
    return (result);
}

//todo: show more information!
inline string warning_not_deterministic(const string& label_id) {
    return "Preconditions of transitions with label \"" + label_id +
            "\" do not guarantee determinism.";
}

inline string warning_reseted_clocks_input(const string &label_id) {
    return "Transitions of input label \"" + label_id +
            "\" must reset the same clocks to ensure determinism.";
}

inline string warning_reseted_clocks_output(const string &label_id,
                                            const string &clock_id) {
    return "Transitions of output label \"" + label_id +
            "\" must reset the same clocks to ensure determinism,"
            " since they are enabled by the same clock \"" + clock_id +
            "\".";
}

inline string warning_same_clock_different_label(const string &clock_id,
                                                 const string &label1_id,
                                                 const string &label2_id) {
    return "Transitions of output labels \"" + label1_id +
            "\" and \"" + label2_id + "\" are enabled by the same clock \""
            + clock_id  + "\", which is a potential source of non-determinism";
}

} //namespace

z3::sort Z3Converter::type_to_sort(Type type, z3::context& ctx) {
    switch (type) {
    case Type::tbool: return ctx.bool_sort();
    case Type::tint: return ctx.int_sort();
    case Type::tfloat: return ctx.real_sort();
    default:
        throw_FigException("Unsupported type");
    }
}

std::function<z3::expr (z3::expr)> Z3Converter::uop_to_fun(ExpOp op) {
    switch(op) {
        case ExpOp::nott: return [] (const z3::expr &a) {return (! a);};
        case ExpOp::minus: return [] (const z3::expr &a) {return (- a);};
        default:
            throw_FigException("Unsupported unary operator");
    }
}

std::function<z3::expr (z3::expr, z3::expr)>
Z3Converter::bop_to_fun(ExpOp op) {
    switch(op) {
    case ExpOp::andd: return [] (const z3::expr &a, const z3::expr &b) {
            return (a && b);
        };
    case ExpOp::orr: return [] (const z3::expr &a, const z3::expr &b) {
            return (a || b);
        };
    case ExpOp::eq: return [] (const z3::expr &a, const z3::expr &b) {
            return (a == b);
        };
    case ExpOp::neq: return [] (const z3::expr &a, const z3::expr &b) {
            return (a != b);
        };
    case ExpOp::lt: return [] (const z3::expr &a, const z3::expr &b) {
            return (a < b);
        };
    case ExpOp::le: return [] (const z3::expr &a, const z3::expr &b) {
            return (a <= b);
        };
    case ExpOp::gt: return [] (const z3::expr &a, const z3::expr &b) {
            return (a > b);
        };
    case ExpOp::ge: return [] (const z3::expr &a, const z3::expr &b) {
            return (a >= b);
        };
    case ExpOp::plus: return [] (const z3::expr &a, const z3::expr &b) {
            return (a + b);
        };
    case ExpOp::minus: return [] (const z3::expr &a, const z3::expr &b) {
            return (a - b);
        };
    case ExpOp::times: return [] (const z3::expr &a, const z3::expr &b) {
            return (a * b);
        };
    case ExpOp::div: return [] (const z3::expr &a, const z3::expr &b) {
            return (a / b);
        };
    case ExpOp::mod: return [] (const z3::expr &a, const z3::expr &b) {
            return (::z3mod(a, b));
        };
    default:
        throw_FigException("Unsupported operator");
    }
}

void Z3Converter::visit(shared_ptr<IConst> node) {
    expression = context->int_val(node->value);
}

void Z3Converter::visit(shared_ptr<BConst> node) {
    expression = context->bool_val(node->value);
}

void Z3Converter::visit(shared_ptr<FConst> node) {
    // context->real_val takes a fraction
    // @todo: convert the float to fraction.
    (void) node;
    throw_FigException("Unsupported float constants");
}

void Z3Converter::visit(shared_ptr<LocExp> node) {
    ExpEvaluator ev;
    node->accept(ev);
    if (!ev.has_errors()) {
        //set the expression to the result of the evaluation
        ev.value_to_ast()->accept(*this);
    } else {
        const string &name = node->location->id;
        z3::sort sort = type_to_sort(node->type, *context);
        expression = context->constant(name.c_str(), sort);
        names.insert(name);
        if (sorts.find(name) == sorts.end()) {
            sorts[name] = sort;
        }
    }
}

void Z3Converter::visit(shared_ptr<OpExp> node) {
    if (node->arity == Arity::one) {
        node->left->accept(*this);
        z3::expr left = expression;
        expression = uop_to_fun(node->bop) (left);
    } else if (node->arity == Arity::two) {
        node->left->accept(*this);
        z3::expr left = expression;
        node->right->accept(*this);
        z3::expr right = expression;
        expression = bop_to_fun(node->bop) (left, right);
    }
}

std::set<string> Z3Converter::get_names() {
    return (names);
}

z3::expr Z3Converter::get_expression() {
    return (expression);
}

z3::expr ModelVerifier::eval_and_convert(shared_ptr<Exp> exp) {
    Z3Converter conv (context);
    eval_or_throw(exp)->accept(conv);
    return (conv.get_expression());
}

z3::expr ModelVerifier::convert(shared_ptr<Exp> exp) {
    Z3Converter conv (context);
    exp->accept(conv);
    return (conv.get_expression());
}

z3::expr ModelVerifier::convert(shared_ptr<Exp> exp, std::set<string>& names) {
    Z3Converter conv (context);
    exp->accept(conv);
    for (auto name : conv.get_names()) {
        names.insert(name);
    }
    return (conv.get_expression());
}

void ModelVerifier::add_names_limits(const std::set<string> &names) {
    for (const string& name : names) {
        shared_ptr<Decl> decl = current_scope->local_decls.at(name);
        if (decl->has_range()) {
            const auto& sort = Z3Converter::type_to_sort(decl->type, *context);
            z3::expr low = eval_and_convert(decl->lower);
            z3::expr up  = eval_and_convert(decl->upper);
            solver->add(context->constant(name.c_str(), sort) >= low);
            solver->add(context->constant(name.c_str(), sort) <= up);
        }
    }
}

void ModelVerifier::debug_print_solver() {
    std::cout << Z3_solver_to_string(*context, *solver) << std::endl;
}

void ModelVerifier::check_output_determinism(const string &clock_id) {
    auto &tr_actions = current_scope->triggered_actions;
    auto range = tr_actions.equal_range(clock_id);
    auto it1 = range.first;
    while (it1 != range.second && !has_warnings()) {
        auto it2 = next(it1);
        while (it2 != range.second && !has_warnings()) {
            solver->push();
            shared_ptr<Action> a1 = ((*it1).second);
            shared_ptr<Action> a2 = ((*it2).second);
            std::set<string> names;
            //both preconditions valid:
            shared_ptr<Exp> guard1 = a1->guard;
            solver->add(convert(guard1, names));
            shared_ptr<Exp> guard2 = a2->guard;
            solver->add(convert(guard2, names));
            add_names_limits(names);
            const string &label1_id = a1->id;
            const string &label2_id = a2->id;
            if (solver->check()) {
                //this two transition potentially enabled by the same clock
                //let's check that at least will produce the same output
                if (label1_id != label2_id) {
                    put_warning(::warning_same_clock_different_label(clock_id,
                                label1_id,
                                label2_id));
                } else {
                    //now let's check if both transitions reset the same clocks
                    bool same_clocks = ::resets_clocks_of(a1, a2) &&
                            resets_clocks_of(a2, a1);
                    if (!same_clocks) {
                        put_warning(::warning_reseted_clocks_output(label1_id,
                                                                    clock_id));
                    }
                    //now let's check that resulting state is the same
                    if (!has_warnings()) {
                        check_rhs(a1, a2);
                        check_rhs(a2, a1);
                    }
                }
            }
            solver->pop();
            it2++;
        }
        it1++;
    }
}

void ModelVerifier::check_input_determinism(const string &label_id) {
    auto &label_actions = current_scope->label_actions;
    auto range = label_actions.equal_range(label_id);
    auto it1 = range.first;
    while (it1 != range.second && !has_warnings()) {
        auto it2 = next(it1);
        while (it2 != range.second && !has_warnings()) {
            solver->push();
            shared_ptr<Action> a1 = ((*it1).second);
            shared_ptr<Action> a2 = ((*it2).second);
            std::set<string> names;
            //both preconditions valid:
            shared_ptr<Exp> guard1 = a1->guard;
            solver->add(convert(guard1, names));
            shared_ptr<Exp> guard2 = a2->guard;
            solver->add(convert(guard2, names));
            add_names_limits(names);
            if (solver->check()) {
                // there is non-determinism, but it could be safe.
                // Let's check that the postcondition is really different
                check_rhs(a1, a2); //assignments of a1 equivalent to those in a2
                check_rhs(a2, a1); //vice versa!
                // now check that they reset the same clocks
                if (!has_warnings()) {
                    //now let's check if both reset the same clocks
                    bool same_clocks = ::resets_clocks_of(a1, a2) &&
                            ::resets_clocks_of(a2, a1);
                    if (!same_clocks) {
                        put_warning(::warning_reseted_clocks_input(label_id));
                    }
                }
            }
            solver->pop();
            it2++;
        }
        it1++;
    }
}

unsigned int ModelVerifier::label_transitions_num(const string& label_id) {
    return (current_scope->label_actions.count(label_id));
}

void ModelVerifier::visit(shared_ptr<Model> model) {
    auto& bodies = model->get_modules();
    auto& ids = model->get_modules_ids();
    unsigned int i = 0;
    while (i < bodies.size() && !has_warnings()) {
        const string &id = ids[i];
        current_scope = ModuleScope::scopes.at(id);
        check_input_determinism_all();
        solver->reset();
        check_output_determinism_all();
        i++;
    }
}

void ModelVerifier::check_output_determinism_all() {
    map<string, shared_ptr<Dist>> &clocks_dist = current_scope->clock_dists;
    auto it = clocks_dist.begin();
    while (it != clocks_dist.end() && !has_warnings()) {
        check_output_determinism((*it).first);
        it++;
    }
}

void ModelVerifier::check_input_determinism_all() {
    map<string, LabelType> &labels_type = current_scope->labels;
    map<string, LabelType>::iterator it = labels_type.begin();
    while (it != labels_type.end() && !has_warnings()) {
        if ((*it).second == LabelType::in) {
            const string &label_id = (*it).first;
            check_input_determinism(label_id);
        }
        it++;
    }
}

void ModelVerifier::check_rhs(shared_ptr<Action> a1, shared_ptr<Action> a2) {
    auto& effects = a1->effects;
    auto it = effects.begin();
    while (it != effects.end() && !has_warnings()) {
        shared_ptr<Effect> effect = *it;
        if (effect->is_state_change()) {
            shared_ptr<Exp> arg1 = effect->arg;
            Type state_type = arg1->type;
            const string &state_id = effect->loc->id;
            //check if the other transition also modifies "state_id"
            shared_ptr<Exp> arg2 = find_assignment_rhs(a2, state_id);
            solver->push();
            if (arg2 == nullptr) {
                //the other transition does not change 'state_id'
                const auto &sort =
                        Z3Converter::type_to_sort(state_type, *context);
                const auto &state = context->constant(state_id.c_str(), sort);
                // assert that this assignment is also an skip
                solver->add(convert(arg1) != state);
            } else {
                solver->add(convert(arg1) != convert(arg2));
            }
            if(solver->check()) {
                put_warning(::warning_not_deterministic(a1->id));
            }
            solver->pop();
        }
        it++;
    }
}

z3::expr ModelVerifier::convert_and_rename(shared_ptr<Exp> exp,
                                           std::set<string>& names_set) {
    Z3Converter conv(*context);
    exp->accept(conv);
    z3::expr z3exp = conv.get_expression();
    std::vector<string> names = conv.get_names();
    std::vector<z3::expr> old_names;
    std::vector<z3::expr> new_names;
    for (unsigned int i = 0; i < names.size(); i++) {
        z3::sort &sort = conv.get_sort_of(names[i]);
        old_names[i] = context->constant(names[i].c_str(), sort);
        names[i] = names[i].push_back('\'');
        names_set.insert(names[i]);
        new_names[i] = context->constant(names[i].c_str(), sort);
    }
    z3::expr result = z3exp.substitute(old_names, new_names);
    return (result);
}

void ModelVerifier
::add_assignments_as_equalities(shared_vector<Effect> effects) {
    for (shared_ptr<Effect> curr : effects) {
        if (curr->is_state_change()) {
            string state_variable = curr->loc->id;
            Type type = current_scope->labels.at(state_variable);
            z3::sort sort = Z3Converter::type_to_sort(type, *context);
            state_variable.push_back('\'');
            z3::expr sv = context->constant(state_variable, sort);
            solver->add(sv == convert(curr->arg));
        }
    }
}

void ModelVerifier ::check_exhausted_clocks(const string &clock_id) {
    auto &tr_actions = current_scope->triggered_actions;
    auto range = tr_actions.equal_range(clock_id);
    auto it1 = range.first;
    auto &actions = current_scope->body->get_actions();
    while (it1 != range.second && !has_warnings()) {
        solver->push();
        auto it2 = actions.begin();
        shared_ptr<Action> a1 = *it1;
        std::set<string> names;
        shared_ptr<Exp> guard1 = a1->guard;
        solver->add(convert_and_rename(guard1, names));
        while (it2 != actions.end() && !has_warnings()) {
            shared_ptr<Action> a2 = *it2;
            if (!resets_clock(a2, clock_id)) {

            }
            it2++;
        }
        it1++;
    }
}





