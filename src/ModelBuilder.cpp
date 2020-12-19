/* Leonardo Rodríguez */

// C
#include <cassert>
#include <cstdlib>  // std::strtoul
// C++
#include <tuple>
#include <memory>  // std::dynamic_pointer_cast<>
#include <type_traits>
// fig
#include <Util.h>
#include <FigLog.h>
#include <Label.h>
#include <ModelSuite.h>
#include <PropertyTransient.h>
#include <PropertyRate.h>
#include <PropertyTBoundSS.h>
#include <ModelBuilder.h>
#include <ExpEvaluator.h>
#include <ExpStateEvaluator.h>
#include <ModelPrinter.h>
#include <ModuleInstance.h>
#include <Transition.h>
#include <Property.h>
#include <PropertyTransient.h>
#include <PropertyRate.h>
#include <PropertyTBoundSS.h>

#if __cplusplus < 201103L
#  error "C++11 standard required, please compile with -std=c++11 or greater\n"
#endif
#if __cplusplus >= 201402L
using std::make_unique;
#endif

using std::set;
using std::shared_ptr;
using std::unique_ptr;
using fig::figTechLog;
using fig::Label;
using fig::Precondition;
using fig::Postcondition;
using fig::Property;
using fig::PropertyRate;
using fig::PropertyTransient;
using fig::PropertyTBoundSS;
using fig::ModuleInstance;
using fig::ModelSuite;
using fig::State;


namespace  // // // // // // // // // // // // // // // // // // // // // //
{

inline const string mb_error_irr(const Type& type) {
    return (" not reducible to " + ModelPrinter::to_str(type) +
            " at compilation time");
}

inline const string mb_error_dist_1(const string &clock_id) {
	return ("First parameter for the distribution of clock " + clock_id +
            mb_error_irr(Type::tfloat));
}

inline const string mb_error_dist_2(const string &clock_id) {
	return ("Second parameter for the distribution of clock " + clock_id +
			mb_error_irr(Type::tfloat));
}

inline const string mb_error_dist_3(const string &clock_id) {
	return ("Third parameter for the distribution of clock " + clock_id +
            mb_error_irr(Type::tfloat));
}

inline const string mb_error_range_1(const string &var_id) {
    return ("Lower bound of range for " + var_id +
            mb_error_irr(Type::tint));
}

inline const string mb_error_range_2(const string &var_id) {
    return ("Upper bound of range for " + var_id +
            mb_error_irr(Type::tint));
}

inline const string mb_error_init(const string &var_id, const Type& type) {
    return ("Initialization of " + var_id +
            mb_error_irr(type));
}

inline const string mb_error_pbranch() {
	return ("In the probability value of a transition branch: "
	        + mb_error_irr(Type::tfloat));
}

} // namespace  // // // // // // // // // // // // // // // // // // // // //


ModelBuilder::ModelBuilder() :
    model_suite(fig::ModelSuite::get_instance())
{ /* Not much to do around here */ }

ModelBuilder::~ModelBuilder() {}

std::map<int, shared_ptr<Prop>> ModelBuilder::property_ast;

inline void ModelBuilder::accept_cond(shared_ptr<ModelAST> node) {
    if (!has_errors()) {
        node->accept(*this);
    }
}

inline void ModelBuilder::accept_visitor(shared_ptr<ModelAST> node,
                                         Visitor& visitor) {
    if (!has_errors()) {
        node->accept(visitor);
    }
}

inline int ModelBuilder::get_int_or_error(shared_ptr<Exp> exp,
                                          const string &msg) {
    int res = 0;
    ExpEvaluator ev (current_scope);
    accept_visitor(exp, ev);
    if (ev.has_type_int()) {
        res = ev.get_int();
    } else {
        put_error(msg);
    }
    return (res);
}

inline float ModelBuilder::get_float_or_error(shared_ptr<Exp> exp,
                                              const string &msg) {
    float res = 0;
    ExpEvaluator ev (current_scope);
    accept_visitor(exp, ev);
    if (ev.has_type_float()) {
        res = ev.get_float();
    } else if (ev.has_type_int()) {
		res = static_cast<float>(ev.get_int());
    } else {
        put_error(msg);
    }
    return (res);
}

inline bool ModelBuilder::get_bool_or_error(shared_ptr<Exp> exp,
                                            const string &msg) {
    bool res = 0;
    ExpEvaluator ev (current_scope);
    accept_visitor(exp, ev);
    if (ev.has_type_bool()) {
        res = ev.get_bool();
    } else {
        put_error(msg);
    }
    return (res);
}

namespace {

inline void dump_precondition_info(const string &expr,
                                   const vector<string> &names) {
	figTechLog << "Precondition" << std::endl;
	figTechLog << "Expr = " << expr << std::endl;
	figTechLog << "Names = ";
    for (auto &name : names) {
		figTechLog << name << " ";
    }
	figTechLog << std::endl;
	figTechLog << "EndPrecondition" << std::endl;
	figTechLog << std::endl;
}

inline void dump_postcondition_info(const string &updates,
                                    const vector<string> &updates_names,
                                    const vector<string> &vars_to_change) {
	figTechLog << "Postcondition" << std::endl;
	figTechLog << "Updates = " << updates << std::endl;
	figTechLog << "Updates Names = ";
    for (auto &name : updates_names) {
		figTechLog << name << " ";
    }
	figTechLog << std::endl;
	figTechLog << "Vars to change = ";
    for (auto &name : vars_to_change) {
		figTechLog << name << " ";
    }
	figTechLog << std::endl;
	figTechLog << "EndPostcondition" << std::endl;
	figTechLog << std::endl;
}

inline void dump_transition_info(const string &label,
                                 const string &clock_trigger,
                                 const string &pre_str,
                                 const vector<string> &pre_names,
                                 const string &post_update,
                                 const vector<string> &post_update_names,
                                 const vector<string> &post_vars_to_change,
                                 const set<string> &clocks_to_reset) {
	figTechLog << "Transition" << std::endl;
	figTechLog << "Label = " << label << std::endl;
	figTechLog << "Clock = " << clock_trigger << std::endl;
    dump_precondition_info(pre_str, pre_names);
    dump_postcondition_info(post_update,
                            post_update_names, post_vars_to_change);
	figTechLog << "Clocks to reset = ";
    for (auto clock_n : clocks_to_reset) {
		figTechLog << clock_n << " ";
    }
	figTechLog << std::endl;
	figTechLog << "EndTransition" << std::endl;
	figTechLog << std::endl;
}

inline void dump_exp_info(const string &expr, const vector<string> &names) {
	figTechLog << "Expr = " << expr << std::endl;
	figTechLog << "Names = ";
    for (auto name : names) {
		figTechLog << name << " ";
    }
	figTechLog << std::endl;
}

inline void dump_transient_info(const string &left_expr,
                                const vector<string> &left_vec,
                                const string &right_expr,
                                const vector<string> &right_vec) {
	figTechLog << "Property Transient" << std::endl;
    dump_exp_info(left_expr, left_vec);
    dump_exp_info(right_expr, right_vec);
	figTechLog << "EndProperty" << std::endl;
	figTechLog << std::endl;
}

inline void dump_rate_info(const string &expr, const vector<string> &vec) {
	figTechLog << "Property Rate" << std::endl;
    dump_exp_info(expr, vec);
	figTechLog << "EndProperty" << std::endl;
	figTechLog << std::endl;
}

inline void dump_var_info(const string &module, const string &var,
                          int low, int up,
                          int init) {
	figTechLog << "Variable of Module " << module << std::endl;
	figTechLog << "Name = " << var << std::endl;
	figTechLog << "Lower = " << low << std::endl;
	figTechLog << "Upper = " << up << std::endl;
	figTechLog << "Init  = " << init << std::endl;
	figTechLog << "EndVariable" << std::endl;
	figTechLog << std::endl;
}

inline void dump_clock_info(const string &module,
                            const string &name,
                            const string &dist_name,
                            float param1, float param2) {
	figTechLog << "Clock of Module " << module << std::endl;
	figTechLog << "Name = " << name << std::endl;
	figTechLog << "DistName = " << dist_name << std::endl;
	figTechLog << "Param1 = " << param1 << std::endl;
	figTechLog << "Param2 = " << param2 << std::endl;
	figTechLog << "EndClock" << std::endl;
	figTechLog << std::endl;
}

} // namespace  // // // // // // // // // // // // // // // // // // // // //


void ModelBuilder::visit(shared_ptr<Model> model) {
    auto& modules = model->get_modules();
	for (unsigned i = 0u ; i < modules.size() ; i++) {
		current_scope = scopes[modules[i]->get_name()];
		accept_cond(modules[i]);
	}
    for (auto &prop : model->get_props()) {
        current_scope = nullptr;
        accept_cond(prop);
    }
}

void ModelBuilder::visit(shared_ptr<ModuleAST> body) {
	module_clocks = make_unique<vector<fig::Clock>>();
	module_transitions = make_unique<vector<fig::Transition>>();
	module_vars = make_unique<vector<Var>>();
	module_arrays = make_unique<vector<Array>>();
//	std::cerr << "\n\nBuilding module " << unique_id() << "\n\n";  /// @todo TODO erase dbg
//	fig::ExpTranslatorVisitor::locationsPrefix = unique_id();
	for (auto &decl : body->get_local_decls())
        accept_cond(decl);
    if (!has_errors()) {
        auto& clocks = *module_clocks;
        Vars state (*module_vars);
        for (Array &p : *module_arrays) {
            state.append_array(p.first, p.second);
        }
        current_module = make_shared<ModuleInstance>
                (current_scope->get_module_name(), state, clocks);
    }
	// Transitions can't be copied: we add them directly to the
	// current_module instead of accumulating them in a vector
	for (auto &transition : body->get_transitions())
        accept_cond(transition);
	if (body->has_committed_actions())  // hint that this module has urgent actions
        current_module->mark_with_committed();
	if (!has_errors())
        model_suite.add_module(current_module);
//	fig::ExpTranslatorVisitor::locationsPrefix = "";
}

fig::Clock ModelBuilder::build_clock(const std::string& id) {
    shared_ptr<Dist> dist = current_scope->dist_by_clock_map().at(id);
    assert(dist != nullptr);
	const auto distrName = ModelPrinter::to_str(dist->get_type());
	assert(!distrName.empty());
    fig::DistributionParameters params;
    //reduce distribution parameters
    if (dist->has_single_parameter()) {
        auto single = dist->to_single_parameter();
        params[0] =
                get_float_or_error(single->get_parameter(), mb_error_dist_1(id));
        for (unsigned int i = 1; i < params.size(); i++)
            params[i] = 0.0;
	} else if (dist->has_multiple_parameters()) {
        auto multiple = dist->to_multiple_parameter();
        params[0] =
                get_float_or_error(multiple->get_first_parameter(),
                                   mb_error_dist_1(id));
        params[1] = get_float_or_error(multiple->get_second_parameter(),
                                       mb_error_dist_2(id));
		if (dist->num_parameters() == 3ul)
			params[2] = get_float_or_error(multiple->get_third_parameter(),
										   mb_error_dist_3(id));
		for (auto i = dist->num_parameters(); i < params.size(); i++)
            params[i] = 0.0;
	} else {
		throw_FigException("unknown distribution type (how many parameters?)");
	}
//	std::cerr << "Building clock " << unique_id(id) << "\n";  /// @todo TODO erase dbg
	// TODO: make ctor take the Distribution object directly, not its name.
	return fig::Clock(unique_id(id), distrName, params);
}

std::string ModelBuilder::unique_id(const string& id) const noexcept {
	if (nullptr == current_scope)
		return id;
	else
		//return current_scope->get_module_name()+"."+id;
		return id;  /// @todo TODO ^^^ module-unique names: the hard part is changing locations names in Expressions, interpreted by Exprtk while building Pre/Postconditions
}

void ModelBuilder::visit(shared_ptr<ArrayDecl> decl) {
	assert(current_scope != nullptr);
	const std::string uniqueID(unique_id(decl->get_id()));
//	const std::string& arrayId = decl->get_id();
	ArrayData data = decl->get_data();
	int size = data.data_size;
	int lower = data.data_min;
	int upper = data.data_max;
	if(has_errors())
		return;
	assert(size > 0);
	std::vector<Var> entries;
	entries.reserve(static_cast<size_t>(size));
	for (auto i = 0ul; i < static_cast<size_t>(size); i++) {
		std::string name = uniqueID + "[" + std::to_string(i) + "]";
		int initValue = data.data_inits[i];
		entries.push_back(std::make_tuple(name, lower, upper, initValue));
	}
	module_arrays->push_back(std::make_pair(uniqueID, entries));
}

void ModelBuilder::visit(shared_ptr<RangedDecl> decl) {
    assert(!decl->is_constant());
    int lower = get_int_or_error(decl->get_lower_bound(),
                                 mb_error_range_1(decl->get_id()));
    int upper = get_int_or_error(decl->get_upper_bound(),
                                 mb_error_range_2(decl->get_id()));
    int value = get_int_or_error(decl->get_init(), decl->get_id());
    if (!has_errors()) {
//		std::cerr << "Building variable " << unique_id(decl->get_id()) << "\n";  /// @todo TODO erase dbg
		const auto &var = make_tuple(unique_id(decl->get_id()), lower, upper, value);
        assert(module_vars != nullptr);
        module_vars->push_back(var);
    }
}

void ModelBuilder::visit(shared_ptr<InitializedDecl> decl) {
	if (decl->is_constant())
		return;  // constans where already reduced. ignore them.
    int lower = 0;
    int upper = 0;
    int value = 0;
    Type type = decl->get_type();
    shared_ptr<Exp> initexp = decl->get_init();
	const auto uniqueID(unique_id(decl->get_id()));
	if (type == Type::tint) {
		value = get_int_or_error(initexp, mb_error_init(uniqueID, type));
        //kind of a constant.
        // q : int init 0;
        lower = value;
        upper = value;
    } else if (type == Type::tbool) {
        lower = 0;
        upper = 1;
		bool res = get_bool_or_error(initexp, mb_error_init(uniqueID, type));
        value = res ? 1 : 0;
    } else if (type == Type::tfloat) {
		throw_FigException("Declaration of float"
		                   " unsupported: \"" + uniqueID + "\"");
    }
    if (!has_errors()) {
//		std::cerr << "Building initialised variable " << uniqueID << "\n";  /// @todo TODO erase dbg
		const auto &var = make_tuple(uniqueID, lower, upper, value);
        assert(module_vars != nullptr);
        module_vars->push_back(var);
    }
}

void ModelBuilder::visit(shared_ptr<ClockDecl> decl) {
//	const auto uniqueID(current_scope->get_module_name()+decl->get_id());
	// NOTE: the current_module name is prepended to the clock's in build_clock
	module_clocks->emplace_back(build_clock(decl->get_id()));
	//module_clocks->push_back(build_clock(decl->get_id()));
}

Label build_label(string&& id, LabelType type) {
    switch(type) {
    case LabelType::in : return Label::make_input(id);
    case LabelType::out: return Label::make_output(id);
	case LabelType::tau: return Label::make_tau();
	case LabelType::in_committed : return Label::make_in_committed(id);
	case LabelType::out_committed: return Label::make_out_committed(id);
	}
	throw_FigException("Unknown type for label "+id);
}

void ModelBuilder::visit(shared_ptr<TransitionAST> tra) {
	assert(nullptr != current_module);
	Label label = build_label(tra->get_label(), tra->get_label_type());  // copy elision
	// Transition constructor expects the id of the triggering clock
	string trigClkName("");
	if (tra->has_triggering_clock())
		trigClkName = unique_id(tra->to_output()->get_triggering_clock()->get_identifier());
	// Populate this transition fields by visiting all probabilistic branches
	assert(0ul < tra->get_num_branches());
	decltype(branches_probabilities)().swap(branches_probabilities);
	decltype(branches_assignments)().swap(branches_assignments);
	decltype(branches_reset_clocks)().swap(branches_reset_clocks);
	for (auto& pbranch: tra->get_branches())
		visit(pbranch);
	assert(branches_probabilities.size() == tra->get_num_branches());
	assert(branches_assignments.size() == tra->get_num_branches());
	assert(branches_reset_clocks.size() == tra->get_num_branches());
//	std::cerr << "Building trans. for trigClk " << trigClkName << "\n";  /// @todo TODO erase dbg
	// Build transition with gathered data
	current_module->add_transition(label,
	                               trigClkName,
	                               Precondition(tra->get_precondition()),  // copy elision
	                               std::move(branches_probabilities),
	                               std::move(branches_assignments),
	                               std::move(branches_reset_clocks));
}

void ModelBuilder::visit(shared_ptr<PBranch> pbranch) {
	// Extract from this branch...
	// ...its probability weight...
	branches_probabilities.emplace_back(
	            get_float_or_error(pbranch->get_probability(), mb_error_pbranch()));
	// ...its variables assignments...
	current_branch_assignments.clear();
	for (const auto& ass: pbranch->get_assignments())
		accept_cond(ass);
	branches_assignments.emplace_back(fig::Postcondition(current_branch_assignments));
	// ...its clocks (names) resets...
	branches_reset_clocks.emplace_back();
	for (shared_ptr<ClockReset> reset: pbranch->get_clock_resets())
		accept_cond(reset);
}

void ModelBuilder::visit(shared_ptr<ClockReset> reset) {
//	std::cerr << "Building reset clk " << unique_id(reset->get_effect_location()->get_identifier()) << "\n";  /// @todo TODO erase dbg
	branches_reset_clocks.back().insert(
	            unique_id(reset->get_effect_location()->get_identifier()));
}

void ModelBuilder::visit(shared_ptr<Assignment> assignment) {
	current_branch_assignments.emplace_back(assignment);
}

void ModelBuilder::visit(shared_ptr<TransientProp> prop) {
    auto property = make_shared<PropertyTransient>
            (prop->get_left(), prop->get_right());
    if (!has_errors()) {
        int id = property->get_id();
        assert(property_ast.find(id) == property_ast.end());
        property_ast[id] = prop;
        model_suite.add_property(property);
    }
}

void ModelBuilder::visit(shared_ptr<RateProp> prop) {
	shared_ptr<Property> property
	        = make_shared<PropertyRate>(prop->get_expression());
	if (!has_errors()) {
		int id = property->get_id();
		assert(property_ast.find(id) == property_ast.end());
		property_ast[id] = prop;
		model_suite.add_property(property);
	}
}

void ModelBuilder::visit(shared_ptr<TBoundSSProp> prop) {
	auto tbound_low = std::dynamic_pointer_cast<IConst>(prop->get_tbound_low());
	auto tbound_upp = std::dynamic_pointer_cast<IConst>(prop->get_tbound_upp());
	shared_ptr<Property> property
	        = make_shared<PropertyTBoundSS>(tbound_low,
	                                        tbound_upp,
	                                        prop->get_expression());
    if (!has_errors()) {
        int id = property->get_id();
        assert(property_ast.find(id) == property_ast.end());
        property_ast[id] = prop;
        model_suite.add_property(property);
    }
}
