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
	default:
		throw_FigException("invalid label type");
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
  return Operator::operator_string(op);
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
	out << string(ident, identc) << str;
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
	// Modules
	out << endl;
	if (debug) {
		print_idented("Modules:");
	}
	for (auto module: model->get_modules()) {
		if (debug) {
			print_idented("Module \"" + module->get_name() + "\"");
			accept_idented(module);
		} else {
			out << endl;
			module->accept(*this);
		}
	}

	/// @todo TODO continue rewriting from here

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
	if (debug) {
		print_idented("=ModuleBody=");
		print_idented("Local Declarations: ");
	} else {
		out << "module " << body->get_name() << endl;
	}
    for (auto decl : body->get_local_decls()) {
        accept_idented(decl);
	}
	if (debug) {
		print_idented("Actions: ");
	}
    for (auto action : body->get_transitions()) {
        accept_idented(action);
	}
	if (!debug) {
		out << "endmodule" << endl;
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
	// Print declaration
	visit(std::static_pointer_cast<Decl>(decl));
	// Print initialization
	if (debug) {
		print_idented("Range:");
		print_idented("Lower:");
		accept_idented(decl->get_lower_bound());
		print_idented("Upper:");
		accept_idented(decl->get_upper_bound());
	} else {
		out << ": [";
		decl->get_lower_bound()->accept(*this);
		out << "..";
		decl->get_upper_bound()->accept(*this);
		out << "];" << endl;
	}
}

void ModelPrinter::visit(shared_ptr<InitializedDecl> decl) {
	// Print declaration
	visit(std::static_pointer_cast<Decl>(decl));
	// Print initialization
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

void ModelPrinter::visit(shared_ptr<ClockDecl> decl) {
	visit(std::static_pointer_cast<Decl>(decl));
	if (!debug) {
		out << ";" << endl;
	}
}

void ModelPrinter::visit(shared_ptr<ArrayDecl> decl) {
	visit(std::static_pointer_cast<Decl>(decl));
	print_idented("Array Size:");
	accept_idented(decl->get_size());
}

void ModelPrinter::visit(shared_ptr<TransitionAST> action) {
	if (debug) {
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
	} else {
		// Print effects only;
		// label, pre and clock should've been printed in sub-class visitor
		for (auto effect : action->get_assignments()) {
			visit(effect);
			out << ") & ";
		}
		for (auto effect : action->get_clock_resets()) {
			visit(effect);
			out << ") & ";
		}
		if (!action->get_assignments().empty() ||
			!action->get_clock_resets().empty())
			out << "\b\b\b   \b\b\b;\n";
	}
}

void ModelPrinter::visit(shared_ptr<InputTransition> transition) {
	if (debug) {
		visit(std::static_pointer_cast<TransitionAST>(transition));
	} else {
		auto label = transition->get_label();
		label.append(label.back() != '?' ? "?" : "");
		print_idented("[" + label + "] ");
		visit(transition->get_precondition());
		out << std::endl;
		ident++;
		print_idented("->\n");
		print_idented("");
		visit(std::static_pointer_cast<TransitionAST>(transition));
		ident--;
	}
}

void ModelPrinter::visit(shared_ptr<OutputTransition> transition) {
	if (debug) {
		visit(std::static_pointer_cast<TransitionAST>(transition));
		print_idented("Triggering Clock:");
		accept_idented(transition->get_triggering_clock());
	} else {
		auto label = transition->get_label();
		label.append(label.back() != '!' ? "!" : "");
		print_idented("[" + label + "] ");
		visit(transition->get_precondition());
		out << " @ " << transition->get_triggering_clock()->get_identifier();
		out << std::endl;
		ident++;
		print_idented("->\n");
		print_idented("");
		visit(std::static_pointer_cast<TransitionAST>(transition));
		ident--;
	}
}

void ModelPrinter::visit(shared_ptr<TauTransition> transition) {
	if (debug) {
		visit(std::static_pointer_cast<TransitionAST>(transition));
		print_idented("Triggering Clock:");
		accept_idented(transition->get_triggering_clock());
	} else {
		print_idented("[] ");
		visit(transition->get_precondition());
		out << " @ " << transition->get_triggering_clock()->get_identifier();
		out << std::endl;
		ident++;
		print_idented("->\n");
		print_idented("");
		visit(std::static_pointer_cast<TransitionAST>(transition));
		ident--;
	}
}

void ModelPrinter::visit(shared_ptr<Effect> effect) {
	// Print lhs of an effect (rhs handled in derived classes)
	if (debug) {
		print_idented("=Effect=");
		print_idented("Location:");
		accept_idented(effect->get_effect_location());
	} else {
		visit(effect->get_effect_location());  // prints opening '('
		out << "' = ";
	}
}

void ModelPrinter::visit(shared_ptr<Assignment> effect) {
	// First print lhs...
	visit(std::static_pointer_cast<Effect>(effect));
	// ...then rhs
	if (debug) {
		print_idented("Assignment Expression:");
		accept_idented(effect->get_rhs());
	} else {
		visit(effect->get_rhs());
		// lhs should've printed an opening '('
		out << ")";
	}
}

void ModelPrinter::visit(shared_ptr<ClockReset> effect) {
	// First print lhs...
	visit(std::static_pointer_cast<Effect>(effect));
	// ...then rhs
	if (debug) {
		print_idented("Distribution:");
		accept_idented(effect->get_dist());
	} else {
		effect->get_dist()->accept(*this);
		// lhs should've printed an opening '('
		out << ")";
	}
}

void ModelPrinter::visit(shared_ptr<Dist> dist) {
	if (debug) {
		print_idented("=Dist=");
		print_idented("Type: " + to_str(dist->get_type()));
	}
}

void ModelPrinter::visit(shared_ptr<SingleParameterDist> dist) {
	if (debug) {
		visit(std::static_pointer_cast<Dist>(dist));
		print_idented("Parameter:");
		accept_idented(dist->get_parameter());
	} else {
		out << to_str(dist->get_type()) << "(";
		visit(dist->get_parameter());
		out << ")";
	}
}

void ModelPrinter::visit(shared_ptr<MultipleParameterDist> dist) {
    visit(std::static_pointer_cast<Dist>(dist));
    print_idented("Parameter1:");
    accept_idented(dist->get_first_parameter());
    print_idented("Parameter2:");
    accept_idented(dist->get_second_parameter());
}

void ModelPrinter::visit(shared_ptr<Location> loc) {
	if (debug) {
		print_idented("=Location=");
		print_idented("ID: \"" + loc->get_identifier() + "\"");
	} else {
		out << "(" << loc->get_identifier();
	}
}

void ModelPrinter::visit(shared_ptr<ArrayPosition> loc) {
    visit(std::static_pointer_cast<Location>(loc));
    print_idented("Array Position:");
    accept_idented(loc->get_index());
}

void ModelPrinter::visit(shared_ptr<IConst> node) {
	if (debug) {
		print_idented("Int Value: " + std::to_string(node->get_value()));
	} else {
		out << node->get_value();
	}
}

void ModelPrinter::visit(shared_ptr<BConst> node) {
	string value(node->get_value() ? "true" : "false");
	if (debug) {
		print_idented("Bool Value: " + value);
	} else {
		out << value;
	}
}

void ModelPrinter::visit(shared_ptr<FConst> node) {
	if (debug) {
		print_idented("Float Value: " + std::to_string(node->get_value()));
	} else {
		out << node->get_value();
	}
}

void ModelPrinter::visit(shared_ptr<LocExp> node) {
	if (debug) {
		print_idented("Value Of");
		accept_idented(node->get_exp_location());
	} else {
		out << node->get_exp_location()->get_identifier();
	}
}

void ModelPrinter::visit(shared_ptr<OpExp> node) {
	if (debug) {
		print_idented("Operator: " + to_str(node->get_operator()));
	} else {
		out << Operator::operator_string(node->get_operator());
	}
}

void ModelPrinter::visit(shared_ptr<BinOpExp> node) {
	if (debug) {
		visit(std::static_pointer_cast<OpExp>(node));
		if (node->has_inferred_type()) {
		   print_idented("Inferred type: " + node->get_inferred_type().to_string());
		}
		print_idented("First Argument:");
		accept_idented(node->get_first_argument());
		print_idented(("Second Argument:"));
		accept_idented(node->get_second_argument());
	} else {
		auto op(node->get_operator());
		auto opStr(Operator::operator_string(op));
		if (Operator::is_infix_operator(op)) {
			out << "(";
			node->get_first_argument()->accept(*this);
			out << opStr;
			node->get_second_argument()->accept(*this);
			out << ")";
		} else {
			out << opStr;
			out << "(";
			node->get_first_argument()->accept(*this);
			out << ",";
			node->get_second_argument()->accept(*this);
			out << ")";
		}
	}
}

void ModelPrinter::visit(shared_ptr<UnOpExp> node) {
	if (debug) {
		visit(std::static_pointer_cast<OpExp>(node));
		print_idented("Argument:");
		accept_idented(node->get_argument());
	} else {
		auto op(node->get_operator());
		auto opStr(Operator::operator_string(op));
		if (Operator::is_infix_operator(op)) {
			out << "(" << opStr;
			node->get_argument()->accept(*this);
			out << ")";
		} else {
			out << opStr << "(";
			node->get_argument()->accept(*this);
			out << ")";
		}
	}
}

void ModelPrinter::visit(shared_ptr<Exp> node) {
	if (auto bconst = std::dynamic_pointer_cast<BConst>(node)) {
		visit(bconst);
	} else if (auto iconst = std::dynamic_pointer_cast<IConst>(node)) {
		visit(iconst);
	} else if (auto fconst = std::dynamic_pointer_cast<FConst>(node)) {
		visit(fconst);
	} else if (auto locexp = std::dynamic_pointer_cast<LocExp>(node)) {
		visit(locexp);
	} else if (auto binop = std::dynamic_pointer_cast<BinOpExp>(node)) {
		visit(binop);
	} else if (auto unop = std::dynamic_pointer_cast<UnOpExp>(node)) {
		visit(unop);
	} else {
		throw_FigException("invalid Expression type");
	}
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

