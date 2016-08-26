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
    auto& bodies = model->get_modules();
    auto& ids = model->get_modules_ids();
    unsigned int i = 0;
    while (i < bodies.size()) {
        current_scope = scopes[ids[i]];
        accept_cond(bodies[i]);
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
        const Transition &tr {Label {label_id, false}, "", prec, post,
                    vector<string>()};
        current_module->add_transition(tr);
    }
}

void ModelBuilder::visit(shared_ptr<ModuleBody> body) {
    module_vars = make_unique<vector<Var>>();
    module_clocks = make_unique<vector<Clock>>();
    module_transitions = make_unique<vector<Transition>>();
    module_ie_pre = map<string, pair<string, vector<string>>>();
    for (auto &decl : body->get_local_decls()) {
        accept_cond(decl);
    }
    if (!has_errors()) {
        //ensure the same output as Raul's frontend by sorting clocks:
        auto& clocks = *module_clocks;
        current_module = make_shared<ModuleInstance>
                (current_scope->id, *module_vars, clocks);
    }
    //Note: Transitions can't be copied, we need to add them directly to
    // the current_module instead of accumulate them in a vector
    for (auto &action : body->get_actions()) {
        accept_cond(action);
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
    if (dist->arity == Arity::one) {
        params[0] = get_float_or_error(dist->param1, mb_error_dist_1(id));
        for (unsigned int i = 1; i < params.size(); i++) {
            params[i] = 0.0;
        }
    }
    if (dist->arity == Arity::two) {
        params[0] = get_float_or_error(dist->param1, mb_error_dist_1(id));
        params[1] = get_float_or_error(dist->param2, mb_error_dist_2(id));
        for (unsigned int i = 2; i < params.size(); i++) {
            params[i] = 0.0;
        }
    }
    //todo: constructor should accept the Distribution object directly,
    //not the name.
    return Clock(id, ModelPrinter::to_str(dist->type), params);
}

void ModelBuilder::visit(shared_ptr<Decl> decl) {
    int lower = 0;
    int upper = 0;
    int init = 0;
    Type type;
    if (decl->is_array()) {
        put_error("Arrays not yet supported");
    }
    if (decl->has_range()) {
        lower = get_int_or_error(decl->lower, mb_error_range_1(decl->id));
        upper = get_int_or_error(decl->upper, mb_error_range_2(decl->id));
        type = Type::tint;
    } else {
        lower = 0;
        upper = 1;
        type = Type::tbool;
    }
    if (decl->has_single_init()) {
        shared_ptr<Exp> iniexp = decl->inits.at(0);
        if (type == Type::tint) {
            init = get_int_or_error(iniexp, mb_error_init(decl->id, type));
        } else if (type == Type::tbool) {
            bool res = get_bool_or_error(iniexp, mb_error_init(decl->id, type));
            init = res ? 1 : 0;
        } else {
            throw_FigException("Not yet supported declaration type");
        }
        if (!has_errors()) {
            const auto &var = make_tuple(decl->id, lower, upper, init);
            assert(module_vars != nullptr);
            module_vars->push_back(var);
        }
    } else if (decl->type == Type::tclock) {
        module_clocks->push_back(build_clock(decl->id));
    } else {
        throw_FigException("Unsupported declaration.");
    }
}

Label build_label(const string &id, LabelType type) {
    switch(type) {
    case LabelType::in : return Label(id, false);
    case LabelType::out: return Label(id, true);
    case LabelType::commited:
        throw_FigException("Commited actions not yet supported");
    case LabelType::empty: return Label(id, true);
    default:
        throw_FigException("Unsupported label type");
    }
}

void ModelBuilder::update_module_ie(shared_ptr<Action> action) {
    const string &label_id = action->id;
    if (action->type == LabelType::in) {
        auto guard_p = ::exp_desc_pair(action->guard);
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
}

void ModelBuilder::visit(shared_ptr<Action> action) {
    const string &label_id = action->id;
    LabelType label_type = action->type;
    Label label = build_label(label_id, label_type);
    update_module_ie(action);
    //Transition constructor expects the id of the triggering
    //clock,  let's get it:
    string t_clock = action->has_clock() ?
                action->clock_loc->id : std::string();
    //Precondition
    ExpStringBuilder string_builder;
    action->guard->accept(string_builder);
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
    auto &effects = action->get_effects();
    auto it = effects.begin();
    while (it != effects.end()) {
        (*it)->accept(*this);
        //avoid putting a ',' at the end:
        //we could also let the comma be, and then delete it
        if ((it + 1) != effects.end()) {
            const shared_ptr<Effect>& next = *(it + 1);
            if (next->is_state_change()) {
                transition_update << ",";
            }
        }
        it++;
    }
    const string &update = transition_update.str();
    Postcondition post (update, *transition_read_vars, *transition_write_vars);
    assert(current_module != nullptr);
    const Transition &trans {label, t_clock, pre, post, *transition_clocks};
    current_module->add_transition(trans);
}

void ModelBuilder::visit(shared_ptr<Effect> effect) {
    if (effect->is_clock_reset()) {
        transition_clocks->insert(effect->loc->id);
    } else if (effect->is_state_change()) {
        accept_cond(effect->loc);
        ExpStringBuilder str_builder;
        effect->arg->accept(str_builder);
        const vector<string> &names = str_builder.get_names();
//      vector<string>::iterator end = transition_read_vars->end();  // unused!
        transition_read_vars->insert(transition_read_vars->cend(),
                                     names.cbegin(), names.cend());
        transition_write_vars->push_back(effect->loc->id);
        transition_update << str_builder.str();
    }
}

void ModelBuilder::visit(shared_ptr<Prop> prop) {
    ExpStringBuilder left_b;
    prop->left->accept(left_b);
    const vector<string> &left_names = left_b.get_names();
    const string &left_expr = left_b.str();
    shared_ptr<Property> property = nullptr;
    if (prop->type == PropType::transient) {
        ExpStringBuilder right_b;
        prop->right->accept(right_b);
        const vector<string> &right_names = right_b.get_names();
        const string &right_expr = right_b.str();
        property = make_shared<PropertyTransient>
                (left_expr, left_names, right_expr, right_names);

    } else if (prop->type == PropType::rate) {
        property = make_shared<PropertyRate>(left_expr, left_names);
    } else {
        put_error("Unsupported property type");
    }
    if (!has_errors()) {
        int id = property->get_id();
        assert(property_ast.find(id) == property_ast.end());
        property_ast[id] = prop;
        model_suite.add_property(property);
    }
}
