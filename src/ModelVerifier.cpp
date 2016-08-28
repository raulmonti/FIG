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
}

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
        expression = context->constant(node->location->id.c_str(),
                                       type_to_sort(node->type, *context));
        names.insert(node->location->id);
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

void ModelVerifier::convert_then_add(shared_ptr<Exp> exp) {
    Z3Converter conv (context);
    exp->accept(conv);
    solver->add(conv.get_expression());
    add_names_limits(conv.get_names());
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

void ModelVerifier::check_label_preconditions(const string &label_id) {
    auto &label_actions = current_scope->label_actions;
    auto range = label_actions.equal_range(label_id);
    auto it1 = range.first;
    while (it1 != range.second && !has_errors()) {
        auto it2 = next(it1);
        while (it2 != range.second && !has_errors()) {
            solver->push();
            shared_ptr<Exp> guard1 = ((*it1).second)->guard;
            convert_then_add(guard1);
            shared_ptr<Exp> guard2 = ((*it2).second)->guard;
            convert_then_add(guard2);
            if(solver->check()) {
                put_error("Potential non-determinism input label \"" +
                          label_id + "\"");
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
    while (i < bodies.size() && !has_errors()) {
        const string &id = ids[i];
        current_scope = ModuleScope::scopes.at(id);
        check_input_determinism();
        i++;
    }
}

void ModelVerifier::check_input_determinism() {
    map<string, LabelType> &labels_type = current_scope->labels;
    map<string, LabelType>::iterator it = labels_type.begin();
    while (it != labels_type.end() && !has_errors()) {
        if ((*it).second == LabelType::in) {
            const string &label_id = (*it).first;
            check_label_preconditions(label_id);
        }
        it++;
    }
}





