/* Leonardo Rodríguez */

#include <tuple>
#include <cassert>
#include <ModelBuilder.h>
#include <ExpEvaluator.h>
#include <ExpStringBuilder.h>
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
    ExpEvaluator ev;
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
    ExpEvaluator ev;
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
    ExpEvaluator ev;
    accept_visitor(exp, ev);
    if (ev.has_type_bool()) {
        res = ev.get_bool();
    } else {
        put_error(msg);
    }
    return (res);
}

namespace {
std::pair<string, vector<string>>
exp_desc_pair(shared_ptr<Exp> exp) {
    ExpStringBuilder str_b;
    exp->accept(str_b);
    return std::make_pair(str_b.str(), str_b.get_names());
}

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

void ModelBuilder::build_input_enabled() {
    assert(current_module != nullptr);
    for (auto entry : module_ie_pre) {
        const string &label_id = entry.first;
        string &pre_str = entry.second.first;
        const vector<string> &names = entry.second.second;
        Precondition prec {"true& " + pre_str, names};
        //skip:
        Postcondition post {"", vector<string>(), vector<string>()};
        const Transition &tr {Label::make_input(label_id), "", prec, post,
                    vector<string>()};
        current_module->add_transition(tr);
    }
}

void ModelBuilder::visit(shared_ptr<ModuleAST> body) {
    module_vars = make_unique<vector<Var>>();
    module_clocks = make_unique<vector<Clock>>();
    module_transitions = make_unique<vector<Transition>>();
    module_ie_pre = map<string, pair<string, vector<string>>>();
    for (auto &decl : body->get_local_decls()) {
        accept_cond(decl);
    }
    if (!has_errors()) {
        auto& clocks = *module_clocks;
        current_module = make_shared<ModuleInstance>
                (current_scope->id, *module_vars, clocks);
    }
    //Note: Transitions can't be copied, we need to add them directly to
    // the current_module instead of accumulate them in a vector
    for (auto &transition : body->get_transitions()) {
        accept_cond(transition);
    }
    build_input_enabled();
    if (!has_errors()) {
        model_suite.add_module(current_module);
    }
}

Clock ModelBuilder::build_clock(const std::string& id) {
    shared_ptr<Dist> dist = current_scope->clock_dists[id];
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
    (void) decl;
    throw_FigException("Array not yet supported");
}

void ModelBuilder::visit(shared_ptr<RangedDecl> decl) {
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
        throw_FigException("Declaration of float unsupported");
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

Label build_label(const string &id, LabelType type) {
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

void ModelBuilder::update_module_ie(shared_ptr<InputTransition> action) {
    const string &label_id = action->get_label();
    auto guard_p = ::exp_desc_pair(action->get_precondition());
    const string nott_guard = "!(" + guard_p.first + ")";
    auto &names = guard_p.second;
    if (module_ie_pre.find(label_id) == module_ie_pre.end()) {
            module_ie_pre[label_id] = make_pair(nott_guard, names);
    } else {
        auto &pre_p = module_ie_pre[label_id];
        names.insert(names.end(),
                     pre_p.second.cbegin(), pre_p.second.cend());
        module_ie_pre[label_id]
                = make_pair(pre_p.first + "& " + nott_guard, names);
    }
}

void ModelBuilder::visit(shared_ptr<TransitionAST> action) {
    const string &label_id = action->get_label();
    LabelType label_type = action->get_label_type();
    Label label = build_label(label_id, label_type);
    if (label_type == LabelType::in) {
        update_module_ie(action->to_input());
    }
    //Transition constructor expects the id of the triggering
    //clock,  let's get it:
    string t_clock = std::string();
    if (action->has_triggering_clock()) {
        t_clock = action->to_output()->get_triggering_clock()->get_identifier();
    }
    //Precondition
    ExpStringBuilder string_builder;
    action->get_precondition()->accept(string_builder);
    string result = string_builder.str();
    if (result == "true") {
        //Ensure the exact same output as Raul's old parser.
        result = "";
    }
    Precondition pre (result, string_builder.get_names());
    //Postcondition, to build the postcondition we need to visit the effects.
    transition_read_vars = make_unique<vector<string>>();
    transition_write_vars = make_unique<vector<string>>();
    transition_update.str(std::string());
    transition_clocks = make_unique<set<string>>();
    auto &assig = action->get_assignments();
    auto it = assig.begin();
    while (it != assig.end()) {
        (*it)->accept(*this);
        //avoid putting a ',' at the end:
        //we could also let the comma be, and then delete it
        if ((it + 1) != assig.end()) {
                transition_update << ",";
        }
        it++;
    }
    for (auto cr : action->get_clock_resets()) {
        cr->accept(*this);
    }
    const string &update = transition_update.str();
    Postcondition post (update, *transition_read_vars, *transition_write_vars);
    assert(current_module != nullptr);
    const Transition &trans {label, t_clock, pre, post, *transition_clocks};
    current_module->add_transition(trans);
}

void ModelBuilder::visit(shared_ptr<ClockReset> reset) {
    transition_clocks->insert(reset->get_effect_location()->get_identifier());
}

void ModelBuilder::visit(shared_ptr<Assignment> assig) {
    accept_cond(assig->get_effect_location());
    ExpStringBuilder str_builder;
    assig->get_rhs()->accept(str_builder);
    const vector<string> &names = str_builder.get_names();
    transition_read_vars->insert(transition_read_vars->cend(),
                                 names.cbegin(), names.cend());
    transition_write_vars->push_back(
                assig->get_effect_location()->get_identifier());
    transition_update << str_builder.str();
}

void ModelBuilder::visit(shared_ptr<TransientProp> prop) {
    ExpStringBuilder left_b;
    prop->get_left()->accept(left_b);
    const vector<string> &left_names = left_b.get_names();
    const string &left_expr = left_b.str();
    ExpStringBuilder right_b;
    prop->get_right()->accept(right_b);
    const vector<string> &right_names = right_b.get_names();
    const string &right_expr = right_b.str();
    auto property = make_shared<PropertyTransient>
            (left_expr, left_names, right_expr, right_names);
    if (!has_errors()) {
        int id = property->get_id();
        assert(property_ast.find(id) == property_ast.end());
        property_ast[id] = prop;
        model_suite.add_property(property);
    }
}

void ModelBuilder::visit(shared_ptr<RateProp> prop) {
    ExpStringBuilder exp_b;
    prop->get_expression()->accept(exp_b);
    const vector<string> &exp_names = exp_b.get_names();
    const string &exp_str = exp_b.str();
    shared_ptr<Property> property
            = make_shared<PropertyRate>(exp_str, exp_names);
    if (!has_errors()) {
        int id = property->get_id();
        assert(property_ast.find(id) == property_ast.end());
        property_ast[id] = prop;
        model_suite.add_property(property);
    }
}
