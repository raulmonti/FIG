/* Leonardo Rodríguez */

#include "ModelPrinter.h"
#include "FigException.h"
#include "Operators.h"
#include <string>

using std::cout;
using std::endl;

string ModelPrinter::to_str(Type type) {
    return Ty::to_string(type);
}

string ModelPrinter::to_str(LabelType type) {
    string result;
    switch(type) {
    case LabelType::in:
        result = "Input";
        break;
    case LabelType::out:
        result = "Output";
        break;
    case LabelType::out_committed:
        result = "Output Committed";
        break;
    case LabelType::in_committed:
        result = "Input Committed";
        break;
    case LabelType::tau:
        result = "Tau";
        break;
    }
    return result;
}

string ModelPrinter::to_str(DistType type) {
    string result;
    switch(type) {
    case DistType::erlang:
        result = "erlang";
        break;
    case DistType::uniform:
        result = "uniform";
        break;
    case DistType::normal:
        result = "normal";
        break;
    case DistType::exponential:
        result = "exponential";
        break;
    case DistType::lognormal:
        result = "lognormal";
        break;
    case DistType::weibull:
        result = "weibull";
        break;
    case DistType::rayleigh:
        result = "rayleigh";
        break;
    case DistType::gamma:
        result = "gamma";
        break;
	default:
		throw_FigException("invalid clock distribution");
		break;
	}
    return result;
}

string ModelPrinter::to_str(ExpOp op) {
<<<<<<< HEAD
    string result;
    switch(op) {
    case ExpOp::plus: result = "+"; break;
    case ExpOp::times: result = "*"; break;
    case ExpOp::minus: result = "-"; break;
    case ExpOp::div: result = "/"; break;
    case ExpOp::mod: result = "%"; break;
	case ExpOp::implies: result = "->"; break;
    case ExpOp::andd: result = "&"; break;
    case ExpOp::orr: result = "|"; break;
    case ExpOp::nott: result = "!"; break;
    case ExpOp::eq: result = "=="; break;
    case ExpOp::neq: result = "!="; break;
    case ExpOp::lt: result = "<"; break;
    case ExpOp::gt: result = ">"; break;
    case ExpOp::le: result = "<="; break;
    case ExpOp::ge: result = ">="; break;
	default: throw_FigException("invalid expression operator"); break;
    }
    return result;
=======
  return Operator::operator_string(op);
>>>>>>> 79c16fe1502050acbd838552b8f3f484d9a8487e
}

string ModelPrinter::to_str(PropType prop_type) {
    string result;
    switch(prop_type) {
    case PropType::transient: result = "Transient"; break;
    case PropType::rate: result = "Rate"; break;
    }
    return (result);
}

void ModelPrinter::accept_idented(shared_ptr<ModelAST> node) {
    ident++;
    node->accept(*this);
    ident--;
}

void ModelPrinter::print_idented(string str) {
	out << string(ident, '\t') << str;
	if (debug) {
		out << endl;
	}
}

void ModelPrinter::visit(shared_ptr<Model> model) {
	if (debug) {
		print_idented("=Model=");
	}
	// Global constants
	out << endl;
	if (debug) {
		print_idented("Global constants:");
	}
	constant = true;
	for (auto decl : model->get_globals()) {
		decl->accept(*this);
	}
	constant = false;

	/// @todo TODO continue rewriting from here

	// Modules
	out << endl;
	if (debug) {
		print_idented("Modules:");
	}
	auto& bodies = model->get_modules();
    unsigned int i = 0;
    while (i < bodies.size()) {
        print_idented("Module: " + bodies[i]->get_name());
        accept_idented(bodies[i]);
        i++;
	}
	// Properties
	out << endl;
	if (debug) {
		print_idented("Properties:");
	}
	for (auto prop : model->get_props()) {
        accept_idented(prop);
    }
}

void ModelPrinter::visit(shared_ptr<ModuleAST> body) {
    print_idented("=ModuleBody=");
    print_idented("Local Declarations: ");
    for (auto decl : body->get_local_decls()) {
        accept_idented(decl);
    }
    print_idented("Actions: ");
    for (auto action : body->get_transitions()) {
        accept_idented(action);
    }
}

void ModelPrinter::visit(shared_ptr<Decl> decl) {
	if (debug) {
		print_idented("=Decl=");
		print_idented("ID: " + decl->get_id());
		print_idented("Type : " + to_str(decl->get_type()));
		if (constant) {
			print_idented("Is constant");
		}
	} else {
		print_idented(
			(constant ? ("const ") : ("")) +
			to_str(decl->get_type())       +
			" " + decl->get_id());
	}
}

void ModelPrinter::visit(shared_ptr<RangedDecl> decl) {
    visit(std::static_pointer_cast<Decl>(decl));
    print_idented("Range:");
    print_idented("Lower:");
    accept_idented(decl->get_lower_bound());
    print_idented("Upper:");
    accept_idented(decl->get_upper_bound());
}

void ModelPrinter::visit(shared_ptr<ArrayDecl> decl) {
    visit(std::static_pointer_cast<Decl>(decl));
    print_idented("Array Size:");
    accept_idented(decl->get_size());
}

void ModelPrinter::visit(shared_ptr<InitializedDecl> decl) {
    visit(std::static_pointer_cast<Decl>(decl));
	if (debug) {
		print_idented("Init:");
	} else {
		out << " = ";
	}
	accept_idented(decl->get_init());
	if (!debug) {
		out << ";" << endl;
	}
}

void ModelPrinter::visit(shared_ptr<TransitionAST> action) {
    print_idented("=Action=");
    print_idented("Label: " + action->get_label());
    print_idented("Label Type: " + to_str(action->get_label_type()));
    print_idented("Precondition:");
    accept_idented(action->get_precondition());
    print_idented("Assignments:");
    for (auto effect : action->get_assignments()) {
        accept_idented(effect);
    }
    print_idented("Clock Resets:");
    for (auto effect : action->get_clock_resets()) {
        accept_idented(effect);
    }
}

void ModelPrinter::visit(shared_ptr<OutputTransition> transition) {
    visit(std::static_pointer_cast<TransitionAST>(transition));
    print_idented("Triggering Clock:");
    accept_idented(transition->get_triggering_clock());
}

void ModelPrinter::visit(shared_ptr<Effect> effect) {
    print_idented("=Effect=");
    print_idented("Location:");
    accept_idented(effect->get_effect_location());
}

void ModelPrinter::visit(shared_ptr<Assignment> effect) {
    visit(std::static_pointer_cast<Effect>(effect));
    print_idented("Assignment Expression:");
    accept_idented(effect->get_rhs());
}

void ModelPrinter::visit(shared_ptr<ClockReset> effect) {
    visit(std::static_pointer_cast<Effect>(effect));
    print_idented("Distribution:");
    accept_idented(effect->get_dist());
}

void ModelPrinter::visit(shared_ptr<Dist> dist) {
    print_idented("=Dist=");
    print_idented("Type: " + to_str(dist->get_type()));
}

void ModelPrinter::visit(shared_ptr<SingleParameterDist> dist) {
    visit(std::static_pointer_cast<Dist>(dist));
    print_idented("Parameter:");
    accept_idented(dist->get_parameter());
}

void ModelPrinter::visit(shared_ptr<MultipleParameterDist> dist) {
    visit(std::static_pointer_cast<Dist>(dist));
    print_idented("Parameter1:");
    accept_idented(dist->get_first_parameter());
    print_idented("Parameter2:");
    accept_idented(dist->get_second_parameter());
}

void ModelPrinter::visit(shared_ptr<Location> loc) {
    print_idented("=Location=");
    print_idented("ID: \"" + loc->get_identifier() + "\"");
}

void ModelPrinter::visit(shared_ptr<ArrayPosition> loc) {
    visit(std::static_pointer_cast<Location>(loc));
    print_idented("Array Position:");
    accept_idented(loc->get_index());
}

void ModelPrinter::visit(shared_ptr<IConst> node) {
	if (debug)
		print_idented("Int Value: " + std::to_string(node->get_value()));
	else
		out << node->get_value();
}

void ModelPrinter::visit(shared_ptr<BConst> node) {
    if (node->get_value()) {
        print_idented("Bool Value: true");
    } else {
        print_idented("Bool Value: false");
    }
}

void ModelPrinter::visit(shared_ptr<FConst> node) {
    print_idented("Float Value: " + std::to_string(node->get_value()));
}

void ModelPrinter::visit(shared_ptr<LocExp> node) {
    print_idented("Value Of");
    accept_idented(node->get_exp_location());
}

void ModelPrinter::visit(shared_ptr<OpExp> node) {
    print_idented("Operator: " + to_str(node->get_operator()));
}

void ModelPrinter::visit(shared_ptr<BinOpExp> node) {
    visit(std::static_pointer_cast<OpExp>(node));
    if (node->has_inferred_type()) {
       print_idented("Inferred type: " + node->get_inferred_type().to_string());
    }
    print_idented("First Argument:");
    accept_idented(node->get_first_argument());
    print_idented(("Second Argument:"));
    accept_idented(node->get_second_argument());
}

void ModelPrinter::visit(shared_ptr<UnOpExp> node) {
    visit(std::static_pointer_cast<OpExp>(node));
    print_idented("Argument:");
    accept_idented(node->get_argument());
}

void ModelPrinter::visit(shared_ptr<Prop> prop) {
    print_idented("=Property=");
    print_idented(to_str(prop->get_type()));
}

void ModelPrinter::visit(shared_ptr<TransientProp> prop) {
    visit(std::static_pointer_cast<Prop>(prop));
    print_idented("Left:");
    accept_idented(prop->get_left());
    print_idented("Right:");
    accept_idented(prop->get_right());
}

void ModelPrinter::visit(shared_ptr<RateProp> prop) {
    visit(std::static_pointer_cast<Prop>(prop));
    print_idented("Expression:");
    accept_idented(prop->get_expression());
}

