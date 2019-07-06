/**
 * @author Leonardo Rodríguez
 * @author Carlos E. Budde (June 2019 updates)
 */

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
	case DistType::hyperexponential2:
		result = "hyperexponential2";
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
	case DistType::dirac:
		result = "dirac";
        break;
	}
    return result;
}

string ModelPrinter::to_str(ExpOp op) {
  return Operator::operator_string(op);
}

string ModelPrinter::to_str(PropType prop_type) {
    string result;
	switch(prop_type)
	{
	case PropType::transient:
		result = "Transient";
		break;
	case PropType::rate:
		result = "Rate";
		break;
	case PropType::tboundss:
		result = "TimeBoundedSS";
		break;
    }
    return (result);
}

void ModelPrinter::accept_indented(shared_ptr<ModelAST> node) {
    ident++;
    node->accept(*this);
    ident--;
}

void ModelPrinter::print_indented(string str = "") {
	out << string(ident, identc) << str;
	if (debug) {
		out << endl;
	}
}

void ModelPrinter::visit(shared_ptr<Model> model) {
	if (debug) {
		print_indented("=Model=");
	}
	// Global constants
	out << endl;
	if (debug) {
		print_indented("Global constants:");
	}
	constant = true;
	for (auto decl : model->get_globals()) {
		decl->accept(*this);
	}
	constant = false;
	// Modules
	out << endl;
	if (debug) {
		print_indented("Modules:");
	}
	for (auto module: model->get_modules()) {
		if (debug) {
			print_indented("Module \"" + module->get_name() + "\"");
			accept_indented(module);
		} else {
			out << endl;
			module->accept(*this);
		}
	}
	// Properties
	out << endl;
	if (debug) {
		print_indented("Properties:");
	} else {
		out << "properties" << endl;
	}
	for (auto prop : model->get_props()) {
		accept_indented(prop);
	}
	if (!debug) {
		out << "endproperties";
	}
	out << endl;
}

void ModelPrinter::visit(shared_ptr<ModuleAST> body) {
	if (debug) {
		print_indented("=ModuleBody=");
		print_indented("Local Declarations: ");
	} else {
		out << "module " << body->get_name() << endl;
	}
    for (auto decl : body->get_local_decls()) {
		accept_indented(decl);
	}
	if (debug) {
		print_indented("Actions: ");
	}
    for (auto action : body->get_transitions()) {
		accept_indented(action);
	}
	if (!debug) {
		out << "endmodule" << endl;
	}
}

void ModelPrinter::visit(shared_ptr<Decl> decl) {
	if (debug) {
		print_indented("=Decl=");
		print_indented("ID: " + decl->get_id());
		print_indented("Type : " + to_str(decl->get_type()));
		if (constant) {
			print_indented("Is constant");
		}
	} else {
		print_indented(
			(constant ? ("const ") : ("")) +
			to_str(decl->get_type())       +
			" " + decl->get_id());
	}
}

void ModelPrinter::visit(shared_ptr<RangedDecl> decl) {
	if (debug) {
		visit(std::static_pointer_cast<Decl>(decl));
		print_indented("Range:");
		print_indented("Lower:");
		accept_indented(decl->get_lower_bound());
		print_indented("Upper:");
		accept_indented(decl->get_upper_bound());
	} else {
		print_indented(decl->get_id());
		out << " : [";
		decl->get_lower_bound()->accept(*this);
		out << "..";
		decl->get_upper_bound()->accept(*this);
		out << "]";
		if (*decl->get_init() != *decl->get_lower_bound()) {
			out << " init ";
			decl->get_init()->accept(*this);
		}
		out << ";" << endl;
	}
}

void ModelPrinter::visit(shared_ptr<InitializedDecl> decl) {
	// Print declaration
	visit(std::static_pointer_cast<Decl>(decl));
	// Print initialization
	if (debug) {
		print_indented("Init:");
	} else {
		out << " = ";
	}
	accept_indented(decl->get_init());
	if (!debug) {
		out << ";" << endl;
	}
}

void ModelPrinter::visit(shared_ptr<ClockDecl> decl) {
	if (debug) {
		visit(std::static_pointer_cast<Decl>(decl));
	} else {
		print_indented(decl->get_id() + ": clock;\n");
	}
}

void ModelPrinter::visit(shared_ptr<ArrayDecl> decl) {
	visit(std::static_pointer_cast<Decl>(decl));
	print_indented("Array Size:");
	accept_indented(decl->get_size());
}

void ModelPrinter::visit(shared_ptr<TransitionAST> action) {
	if (debug) {
		print_indented("=Transition=");
		print_indented("Label: " + action->get_label());
		print_indented("Label Type: " + to_str(action->get_label_type()));
		print_indented("Precondition:");
		accept_indented(action->get_precondition());
		print_indented("Probabilistic branches:");
		for (const auto& pbranch: action->get_branches())
			accept_indented(pbranch);
	} else {
		// Print probabilistic branches only;
		// label, pre and clock should've been printed in derived-class visitor
		for (const auto& pbranch: action->get_branches()) {
			visit(pbranch);
			out << "\n";
			print_indented("\t +");
		}
		out.seekp(static_cast<long>(out.tellp())-3l);
		out << "; \n";
	}
}

void ModelPrinter::visit(shared_ptr<InputTransition> transition) {
	if (debug) {
		visit(std::static_pointer_cast<TransitionAST>(transition));
	} else {
		print_indented("[" + transition->get_label() + "?] ");
		transition->get_precondition()->accept(*this);
		out << std::endl;
		ident++;
		print_indented("->\n");
		print_indented();
		visit(std::static_pointer_cast<TransitionAST>(transition));
		ident--;
	}
}

void ModelPrinter::visit(shared_ptr<OutputTransition> transition) {
	if (debug) {
		visit(std::static_pointer_cast<TransitionAST>(transition));
		print_indented("Triggering Clock:");
		accept_indented(transition->get_triggering_clock());
	} else {
		print_indented("[" + transition->get_label() + "!] ");
		transition->get_precondition()->accept(*this);
		out << " @ " << transition->get_triggering_clock()->get_identifier();
		out << std::endl;
		ident++;
		print_indented("->\n");
		print_indented();
		visit(std::static_pointer_cast<TransitionAST>(transition));
		ident--;
	}
}

void ModelPrinter::visit(shared_ptr<TauTransition> transition) {
	if (debug) {
		visit(std::static_pointer_cast<TransitionAST>(transition));
		print_indented("Triggering Clock:");
		accept_indented(transition->get_triggering_clock());
	} else {
		print_indented("[] ");
		transition->get_precondition()->accept(*this);
		out << " @ " << transition->get_triggering_clock()->get_identifier();
		out << std::endl;
		ident++;
		print_indented("->\n");
		print_indented();
		visit(std::static_pointer_cast<TransitionAST>(transition));
		ident--;
	}
}

void ModelPrinter::visit(shared_ptr<PBranch> pbranch) {
	if (debug) {
		print_indented("=PBranch=");
		print_indented("Probabilistic weight:");
		accept_indented(pbranch->get_probability());
		print_indented("Assignments:");
		for (const auto& effect: pbranch->get_assignments())
			accept_indented(effect);
		print_indented("Clock Resets:");
		for (const auto& effect: pbranch->get_clock_resets())
			accept_indented(effect);
	} else {
		print_indented(" ");
		visit(std::static_pointer_cast<FConst>(pbranch->get_probability()));
		print_indented(" : " );
		for (const auto& effect: pbranch->get_assignments()) {
			visit(effect);
			out << " & ";
		}
		for (const auto& effect: pbranch->get_clock_resets()) {
			visit(effect);
			out << " & ";
		}
		if (!pbranch->get_assignments().empty()
		        || !pbranch->get_clock_resets().empty()) {
			out.seekp(static_cast<long>(out.tellp())-3l);
			out << "   ";
			out.seekp(static_cast<long>(out.tellp())-3l);
		}
	}
}

void ModelPrinter::visit(shared_ptr<Effect> effect) {
	// Print lhs of an effect (rhs handled in derived classes)
	if (debug) {
		print_indented("=Effect=");
		print_indented("Location:");
		accept_indented(effect->get_effect_location());
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
		print_indented("Assignment Expression:");
		accept_indented(effect->get_rhs());
	} else {
		effect->get_rhs()->accept(*this);
		// lhs should've printed an opening '('
		out << ")";
	}
}

void ModelPrinter::visit(shared_ptr<ClockReset> effect) {
	// First print lhs...
	visit(std::static_pointer_cast<Effect>(effect));
	// ...then rhs
	if (debug) {
		print_indented("Distribution:");
		accept_indented(effect->get_dist());
	} else {
		effect->get_dist()->accept(*this);
		// lhs should've printed an opening '('
		out << ")";
	}
}

void ModelPrinter::visit(shared_ptr<Dist> dist) {
	if (debug) {
		print_indented("=Dist=");
		print_indented("Type: " + to_str(dist->get_type()));
	}
}

void ModelPrinter::visit(shared_ptr<SingleParameterDist> dist) {
	if (debug) {
		visit(std::static_pointer_cast<Dist>(dist));
		print_indented("Parameter:");
		accept_indented(dist->get_parameter());
	} else {
		out << to_str(dist->get_type()) << "(";
		dist->get_parameter()->accept(*this);
		out << ")";
	}
}

void ModelPrinter::visit(shared_ptr<MultipleParameterDist> dist) {
	if (debug) {
		visit(std::static_pointer_cast<Dist>(dist));
		print_indented("Parameter1:");
		accept_indented(dist->get_first_parameter());
		print_indented("Parameter2:");
		accept_indented(dist->get_second_parameter());
		if (dist->num_parameters() == 3) {
			print_indented("Parameter3:");
			accept_indented(dist->get_third_parameter());
		}
	} else {
		out << to_str(dist->get_type()) << "(";
		dist->get_first_parameter()->accept(*this);
		out << ",";
		dist->get_second_parameter()->accept(*this);
		if (dist->num_parameters() == 3)
			dist->get_third_parameter()->accept(*this);
		out << ")";
	}
}

void ModelPrinter::visit(shared_ptr<Location> loc) {
	if (debug) {
		print_indented("=Location=");
		print_indented("ID: \"" + loc->get_identifier() + "\"");
	} else {
		out << "(" << loc->get_identifier();
	}
}

void ModelPrinter::visit(shared_ptr<ArrayPosition> loc) {
	if (debug) {
		visit(std::static_pointer_cast<Location>(loc));
		print_indented("Array Position:");
		accept_indented(loc->get_index());
	} else {
		throw_FigException("array printing not inplemented yet in non-debug mode");
	}
}

void ModelPrinter::visit(shared_ptr<IConst> node) {
	if (debug) {
		print_indented("Int Value: " + std::to_string(node->get_value()));
	} else {
		out << node->get_value();
	}
}

void ModelPrinter::visit(shared_ptr<BConst> node) {
	string value(node->get_value() ? "true" : "false");
	if (debug) {
		print_indented("Bool Value: " + value);
	} else {
		out << value;
	}
}

void ModelPrinter::visit(shared_ptr<FConst> node) {
	if (debug) {
		print_indented("Float Value: " + std::to_string(node->get_value()));
	} else {
		out << node->get_value();
	}
}

void ModelPrinter::visit(shared_ptr<LocExp> node) {
	if (debug) {
		print_indented("Value Of");
		accept_indented(node->get_exp_location());
	} else {
		out << node->get_exp_location()->get_identifier();
	}
}

void ModelPrinter::visit(shared_ptr<OpExp> node) {
	if (debug) {
		print_indented("Operator: " + to_str(node->get_operator()));
	} else {
		out << Operator::operator_string(node->get_operator());
	}
}

void ModelPrinter::visit(shared_ptr<BinOpExp> node) {
	if (debug) {
		visit(std::static_pointer_cast<OpExp>(node));
		if (node->has_inferred_type()) {
		   print_indented("Inferred type: " + node->get_inferred_type().to_string());
		}
		print_indented("First Argument:");
		accept_indented(node->get_first_argument());
		print_indented(("Second Argument:"));
		accept_indented(node->get_second_argument());
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
		print_indented("Argument:");
		accept_indented(node->get_argument());
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

void ModelPrinter::visit(shared_ptr<Prop> prop) {
	if (debug) {
		print_indented("=Property=");
		print_indented(to_str(prop->get_type()));
	}
}

void ModelPrinter::visit(shared_ptr<TransientProp> prop) {
	if (debug) {
		visit(std::static_pointer_cast<Prop>(prop));
		print_indented("Left:");
		accept_indented(prop->get_left());
		print_indented("Right:");
		accept_indented(prop->get_right());
	} else {
		print_indented("P( ");
		prop->get_left()->accept(*this);
		out << " U ";
		prop->get_right()->accept(*this);
		out << " )\n";
	}
}

void ModelPrinter::visit(shared_ptr<RateProp> prop) {
	if (debug) {
		visit(std::static_pointer_cast<Prop>(prop));
		print_indented("Expression:");
		accept_indented(prop->get_expression());
	} else {
		print_indented("S( ");
		prop->get_expression()->accept(*this);
		out << " )\n";
	}
}

void ModelPrinter::visit(shared_ptr<TBoundSSProp> prop) {
	if (debug) {
		visit(std::static_pointer_cast<Prop>(prop));
		print_indented("Time-bound low:");
		accept_indented(prop->get_tbound_low());
		print_indented("Time-bound upp:");
		accept_indented(prop->get_tbound_upp());
		print_indented("Expression:");
		accept_indented(prop->get_expression());
	} else {
		print_indented("B( ");
		prop->get_expression()->accept(*this);
		out << " ) [ ";
		prop->get_tbound_low()->accept(*this);
		out << ":";
		prop->get_tbound_upp()->accept(*this);
		out << " ]\n";
	}
}

