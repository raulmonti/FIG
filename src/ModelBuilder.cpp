/* Leonardo Rodríguez */

#include <tuple>
#include <cassert>
#include <ModelBuilder.h>
#include <ExpEvaluator.h>
#include <ModelPrinter.h>


namespace  // // // // // // // // // // // // // // // // // // // // // //
{

inline const string mb_error_irr(const Type& type) {
    return (" not reducible to " + ModelPrinter::to_str(type) +
            " at compilation time");
}

inline const string mb_error_dist_1(const string &clock_id) {
    return ("Fist distribution parameter of clock " + clock_id +
            mb_error_irr(Type::tfloat));
}

inline const string mb_error_dist_2(const string &clock_id) {
    return ("Second distribution parameter of clock " + clock_id +
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
        res = (float) ev.get_int();
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
    std::cout << "Precondition" << std::endl;
    std::cout << "Expr = " << expr << std::endl;
    std::cout << "Names = ";
    for (auto &name : names) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    std::cout << "EndPrecondition" << std::endl;
    std::cout << std::endl;
}

inline void dump_postcondition_info(const string &updates,
                                    const vector<string> &updates_names,
                                    const vector<string> &vars_to_change) {
    std::cout << "Postcondition" << std::endl;
    std::cout << "Updates = " << updates << std::endl;
    std::cout << "Updates Names = ";
    for (auto &name : updates_names) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    std::cout << "Vars to change = ";
    for (auto &name : vars_to_change) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    std::cout << "EndPostcondition" << std::endl;
    std::cout << std::endl;
}

inline void dump_transition_info(const string &label,
                                 const string &clock_trigger,
                                 const string &pre_str,
                                 const vector<string> &pre_names,
                                 const string &post_update,
                                 const vector<string> &post_update_names,
                                 const vector<string> &post_vars_to_change,
                                 const set<string> &clocks_to_reset) {
    std::cout << "Transition" << std::endl;
    std::cout << "Label = " << label << std::endl;
    std::cout << "Clock = " << clock_trigger << std::endl;
    dump_precondition_info(pre_str, pre_names);
    dump_postcondition_info(post_update,
                            post_update_names, post_vars_to_change);
    std::cout << "Clocks to reset = ";
    for (auto clock_n : clocks_to_reset) {
        std::cout << clock_n << " ";
    }
    std::cout << std::endl;
    std::cout << "EndTransition" << std::endl;
    std::cout << std::endl;
}

inline void dump_exp_info(const string &expr, const vector<string> &names) {
    std::cout << "Expr = " << expr << std::endl;
    std::cout << "Names = ";
    for (auto name : names) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
}

inline void dump_transient_info(const string &left_expr,
                                const vector<string> &left_vec,
                                const string &right_expr,
                                const vector<string> &right_vec) {
    std::cout << "Property Transient" << std::endl;
    dump_exp_info(left_expr, left_vec);
    dump_exp_info(right_expr, right_vec);
    std::cout << "EndProperty" << std::endl;
    std::cout << std::endl;
}

inline void dump_rate_info(const string &expr, const vector<string> &vec) {
    std::cout << "Property Rate" << std::endl;
    dump_exp_info(expr, vec);
    std::cout << "EndProperty" << std::endl;
    std::cout << std::endl;
}

inline void dump_var_info(const string &module, const string &var,
                          int low, int up,
                          int init) {
    std::cout << "Variable of Module " << module << std::endl;
    std::cout << "Name = " << var << std::endl;
    std::cout << "Lower = " << low << std::endl;
    std::cout << "Upper = " << up << std::endl;
    std::cout << "Init  = " << init << std::endl;
    std::cout << "EndVariable" << std::endl;
    std::cout << std::endl;
}

inline void dump_clock_info(const string &module,
                            const string &name,
                            const string &dist_name,
                            float param1, float param2) {
    std::cout << "Clock of Module " << module << std::endl;
    std::cout << "Name = " << name << std::endl;
    std::cout << "DistName = " << dist_name << std::endl;
    std::cout << "Param1 = " << param1 << std::endl;
    std::cout << "Param2 = " << param2 << std::endl;
    std::cout << "EndClock" << std::endl;
    std::cout << std::endl;
}
} //namespace

void ModelBuilder::visit(shared_ptr<Model> model) {
    auto& modules = model->get_modules();
    unsigned int i = 0;
    while (i < modules.size()) {
        current_scope = scopes[modules[i]->get_name()];
        accept_cond(modules[i]);
        i++;
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
        for (unsigned int i = 2; i < params.size(); i++) {
            params[i] = 0.0;
        }
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
    default:
        throw_FigException("Unsupported label type");
    }
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
	            std::move(Precondition(action->get_precondition())),
	            std::move(Postcondition(action->get_assignments())),
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
