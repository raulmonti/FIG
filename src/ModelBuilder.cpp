/* Leonardo Rodríguez */

// C++
#include <tuple>
#include <memory>  // std::dynamic_pointer_cast<>
#include <cassert>
#include <cstdlib>  // std::strtoul
// fig
#include <Util.h>
#include <FigLog.h>
#include <Label.h>
#include <State.h>
#include <Clock.h>
#include <ModelBuilder.h>
#include <ExpEvaluator.h>
#include <ModelPrinter.h>
#include <ModuleInstance.h>
#include <Transition.h>
#include <Property.h>
#include <PropertyTransient.h>
#include <PropertyRate.h>
#include <PropertyTBoundSS.h>


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

} // namespace  // // // // // // // // // // // // // // // // // // // // //


ModelBuilder::ModelBuilder() {}
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
    module_clocks = make_unique<vector<Clock>>();
    module_transitions = make_unique<vector<Transition>>();
    module_vars = make_unique<vector<Var>>();
    module_arrays = make_unique<vector<Array>>();
    for (auto &decl : body->get_local_decls()) {
        accept_cond(decl);
    }
    if (!has_errors()) {
        auto& clocks = *module_clocks;
        Vars state (*module_vars);
        for (Array &p : *module_arrays) {
            state.append_array(p.first, p.second);
        }
        current_module = make_shared<ModuleInstance>
                (current_scope->get_module_name(), state, clocks);
    }
    //Note: Transitions can't be copied, we need to add them directly to
    // the current_module instead of accumulate them in a vector
    for (auto &transition : body->get_transitions()) {
        accept_cond(transition);
    }
    if (body->has_committed_actions()) {
        //hint that this module has committed actions
        current_module->mark_with_committed();
    }
    if (!has_errors()) {
        model_suite.add_module(current_module);
    }
}

Clock ModelBuilder::build_clock(const std::string& id) {
    shared_ptr<Dist> dist = current_scope->dist_by_clock_map().at(id);
    assert(dist != nullptr);
    fig::DistributionParameters params;
    //reduce distribution parameters
    if (dist->has_single_parameter()) {
        auto single = dist->to_single_parameter();
        params[0] =
                get_float_or_error(single->get_parameter(), mb_error_dist_1(id));
        for (unsigned int i = 1; i < params.size(); i++) {
            params[i] = 0.0;
        }
    }
    if (dist->has_multiple_parameters()) {
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
    }
    //todo: constructor should accept the Distribution object directly,
    //not the name.
    return Clock(id, ModelPrinter::to_str(dist->get_type()), params);
}

void ModelBuilder::visit(shared_ptr<ArrayDecl> decl) {
    assert(current_scope != nullptr);
    const std::string& arrayId = decl->get_id();
    ArrayData data = decl->get_data();
    int size = data.data_size;
    int lower = data.data_min;
    int upper = data.data_max;
    if(has_errors()) {
        return;
    }
    assert(size > 0);
    std::vector<Var> entries;
    entries.reserve(size);
    for (int i = 0; i < size; i++) {
        std::string name = arrayId + "[" + std::to_string(i) + "]";
        int initValue = data.data_inits[i];
        entries.push_back(std::make_tuple(name, lower, upper, initValue));
    }
    module_arrays->push_back(std::make_pair(arrayId, entries));
}

void ModelBuilder::visit(shared_ptr<RangedDecl> decl) {
    assert(!decl->is_constant());
    int lower = get_int_or_error(decl->get_lower_bound(),
                                 mb_error_range_1(decl->get_id()));
    int upper = get_int_or_error(decl->get_upper_bound(),
                                 mb_error_range_2(decl->get_id()));
    int value = get_int_or_error(decl->get_init(), decl->get_id());
    if (!has_errors()) {
        const auto &var = make_tuple(decl->get_id(), lower, upper, value);
        assert(module_vars != nullptr);
        module_vars->push_back(var);
    }
}

void ModelBuilder::visit(shared_ptr<InitializedDecl> decl) {
    if (decl->is_constant()) {
        //constans where already reduced. ignore them.
        return;
    }
    int lower = 0;
    int upper = 0;
    int value = 0;
    Type type = decl->get_type();
    shared_ptr<Exp> initexp = decl->get_init();
    if (type == Type::tint) {
        value = get_int_or_error(initexp, mb_error_init(decl->get_id(), type));
        //kind of a constant.
        // q : int init 0;
        lower = value;
        upper = value;
    } else if (type == Type::tbool) {
        lower = 0;
        upper = 1;
        bool res =
                get_bool_or_error(initexp, mb_error_init(decl->get_id(), type));
        value = res ? 1 : 0;
    } else if (type == Type::tfloat) {
        throw_FigException("Declaration of float"
                           " unsupported: \"" + decl->get_id() + "\"");
    }
    if (!has_errors()) {
        const auto &var = make_tuple(decl->get_id(), lower, upper, value);
        assert(module_vars != nullptr);
        module_vars->push_back(var);
    }
}

void ModelBuilder::visit(shared_ptr<ClockDecl> decl) {
    module_clocks->push_back(build_clock(decl->get_id()));
}

Label build_label(string&& id, LabelType type) {
    switch(type) {
    case LabelType::in : return Label::make_input(id);
    case LabelType::out: return Label::make_output(id);
    case LabelType::out_committed: return Label::make_out_committed(id);
    case LabelType::in_committed: return Label::make_in_committed(id);
    case LabelType::tau: return Label::make_tau();
    }
	return Label::make_tau();
}

void ModelBuilder::visit(shared_ptr<TransitionAST> action) {
	assert(nullptr != current_module);
	Label label = build_label(action->get_label(),  // rely on copy elision!
	                          action->get_label_type());
    //Transition constructor expects the id of the triggering
    //clock,  let's get it:
	string t_clock("");
	if (action->has_triggering_clock())
        t_clock = action->to_output()->get_triggering_clock()->get_identifier();
    transition_clocks = make_unique<set<string>>();
	for (shared_ptr<ClockReset> reset : action->get_clock_resets())
        accept_cond(reset);
	current_module->add_transition(
	            label,
	            t_clock,
	            Precondition(action->get_precondition()),
	            Postcondition(action->get_assignments()),
	            *transition_clocks);
}

void ModelBuilder::visit(shared_ptr<ClockReset> reset) {
    transition_clocks->insert(reset->get_effect_location()->get_identifier());
}

void ModelBuilder::visit(shared_ptr<Assignment> assig) {
   (void) assig; //do nothing, resolved by parent node.
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
