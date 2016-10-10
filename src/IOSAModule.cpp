/* Leonardo Rodríguez */

#include <queue>

#include "IOSAModule.h"
#include "IOSAExpEval.h"

namespace  iosa {

ModuleIOSA::ModuleIOSA(std::shared_ptr<ModuleAST> ast) {
    this->ast = ast;
    this->scope = ModuleScope::scopes.at(ast->get_name());
    build_initial_state();
    process_transitions();
    auto pr  = [] (Edge<IVert,TransitionInfo> edge) -> void {
        TransitionInfo data = edge.get_data();
        edge.get_src()->print_state(std::cout);
        std::cout << "--[ ";
        std::cout << data.get_label_id();
        std::cout << " ]-->";
        edge.get_dst()->print_state(std::cout);
        std::cout << std::endl;
    };
    print(pr);
    auto pairs = search_non_confluents();
    for (NonConfluentPair pair : pairs) {
        std::cout << " ID0 : " << pair.first.get_data().get_label_id();
        std::cout << " ID1 : " << pair.second.get_data().get_label_id();
        std::cout << "STATE : ";
        pair.first.get_src()->print_state(std::cout);
        std::cout << "NON CONFLUENT!" << std::endl;

    }
}

int ModuleIOSA::get_value(shared_ptr<Exp> exp) const {
    if(!exp->is_constant()) {
        throw_FigException("Expected a constant expression.");
    } else if (exp->get_type() == Type::tfloat) {
        throw_FigException("Float state variables unsupported");
    } else if (exp->get_type() == Type::tbool) {
        shared_ptr<BConst> bexp = std::static_pointer_cast<BConst>(exp);
        return bexp->get_value() ? 1 : 0;
    } else {
        assert(exp->get_type() == Type::tint);
        shared_ptr<IConst> iexp = std::static_pointer_cast<IConst>(exp);
        return iexp->get_value();
    }
}

void ModuleIOSA::add_variable(shared_ptr<Decl> decl) {
    state_value_t low   = 0;
    state_value_t upp   = 0;
    state_value_t value = 0;
    Type type = decl->get_type();
    if (type == Type::tint) {
        assert(decl->has_range());
        shared_ptr<Ranged> range = decl->to_ranged();
        low = get_value(range->get_lower_bound());
        upp = get_value(range->get_upper_bound());
    } else if (type == Type::tbool) {
        low = 0;
        upp = 1;
    } else if (type == Type::tclock) {
        return; //ignore
    } else {
        throw_FigException("Unsupported type at this stage");
    }
    assert(decl->has_init());
    value = get_value(decl->to_initialized()->get_init());
    FixedRange range(low, upp);
    initial_state->add_variable(decl->get_id(), range);
    initial_state->set_variable_value(decl->get_id(), value);
}

void ModuleIOSA::build_initial_state() {
    const shared_map<std::string, Decl> &locals
            = scope->local_decls_map();
    initial_state = make_shared<State>();
    for (auto entry : locals) {
        shared_ptr<Decl> decl = entry.second;
        if (!decl->is_constant()) {
            add_variable(decl);
        }
    }
}

void ModuleIOSA::process_transitions() {
    IVert next = initial_state;
    std::queue<IVert> states;
    states.push(initial_state);
    while (!states.empty()) {
        IVert current = states.front();
        states.pop();
        for (auto entry : scope->transition_by_label_map()) {
            shared_ptr<TransitionAST> tr = entry.second;
            next = process_edge(current, tr);
            if (next != nullptr) {
                states.push(next);
            }
        }
    }
}

bool ModuleIOSA::holds_expression(IVert st, shared_ptr<Exp> bexp) const {
    Evaluator evaluator (st);
    bexp->accept(evaluator);
    return (evaluator.get_value() == 1);
}

IVert ModuleIOSA::process_edge(IVert st, shared_ptr<TransitionAST> transition) {
    IVert result = nullptr;
    shared_ptr<Exp> pre = transition->get_precondition();
    if (holds_expression(st, pre)) {
        shared_vector<Assignment>& as = transition->get_assignments();
        IVert cpy = process_assignments(st, as);
        if (!cpy->is_valid()) {
            throw_FigException("Generated out-of-range state.");
        }
        TransitionInfo tinfo
                (transition->get_label(), transition->get_label_type());
        IEdge edge (st, cpy, tinfo);
        if (!has_edge(edge)) {
            result = cpy;
            add_edge(edge);
        }
    }
    return (result);
}

IVert ModuleIOSA::process_assignments(IVert st,
                                      shared_vector<Assignment>& avec) {
    IVert copy = make_shared<State>(*st);
    for (shared_ptr<Assignment> as : avec) {
        shared_ptr<Exp> rhs = as->get_rhs();
        Evaluator evaluator (st);
        rhs->accept(evaluator);
        std::string name = as->get_effect_location()->get_identifier();
        copy->set_variable_value(name, evaluator.get_value());
    }
    return (copy);
}

std::vector<IEdge>
ModuleIOSA::select_edges_of(IVert src,
                std::function<bool (const IEdge &edge)> prop) {
    auto it = edges.equal_range(src);
    auto fst = it.first;
    auto end = it.second;
    std::vector<IEdge> result;
    while (fst != end) {
        IEdge edge = fst->second;
        if (prop(edge)) {
            result.push_back(edge);
        }
        fst++;
    }
    return (result);
}

std::vector<IEdge> ModuleIOSA::committed_edges_of(IVert st) {
    auto prop = [] (const IEdge &edge) -> bool {
       LabelType type = edge.get_data().get_label_type();
       return (type == LabelType::out_committed);
    };
    return select_edges_of(st, prop);
}

std::vector<IEdge> ModuleIOSA::labeled_edges_of(IVert st, const string &label) {
    auto prop = [label] (const IEdge &edge) -> bool {
        return (edge.get_data().get_label_id() == label);
    };
    return select_edges_of(st, prop);
}

std::vector<NonConfluentPair> ModuleIOSA::search_non_confluents() {
    std::set<IVert, StatePtrComp> visited;
    std::queue<IVert> queue;
    std::vector<NonConfluentPair> result;
    queue.push(initial_state);

    while (!queue.empty()) {
        IVert current = queue.front();
        queue.pop();
        visited.insert(current);
        non_confluents_of(result, current);
        auto it = edges.equal_range(current);
        auto fst = it.first;
        auto end = it.second;
        while (fst != end) {
            IEdge edge = fst->second;
            IVert dst = edge.get_dst();
            if (visited.count(dst) == 0) {
                queue.push(dst);
            }
            fst++;
        }
    }

    return (result);
}

void ModuleIOSA::non_confluents_of(std::vector<NonConfluentPair> &result,
                                   IVert st) {
    std::vector<IEdge> c_edges = committed_edges_of(st);
    state_pos_t i = 0;
    while (i < c_edges.size()) {
        IEdge &edge1 = c_edges[i];
        state_pos_t j = i + 1;
        while (j < c_edges.size()) {
            IEdge edge2 = c_edges[j];
            assert(edge1.get_src() == edge2.get_src());
            if (!edge_confluent(edge1, edge2)) {
                result.push_back(std::make_pair(edge1, edge2));
            }
            j++;
        }
        i++;
    }
}

bool ModuleIOSA::edge_confluent(IEdge &edge1, IEdge &edge2) {
    assert(edge1.get_src() == edge2.get_src());
    string label1 = edge1.get_data().get_label_id();
    string label2 = edge2.get_data().get_label_id();
    IVert dst1 = edge1.get_dst();
    IVert dst2 = edge2.get_dst();

    // edges from dst1 labeled with label2
    std::vector<IEdge> c1 = labeled_edges_of(dst1, label2);
    // edges from dst2 labeled with label1
    std::vector<IEdge> c2 = labeled_edges_of(dst2, label1);

    bool result = false;
    if (!c1.empty() && !c2.empty()) {
        // if one of them is empty, they are non confluent.
        // if they are non-empty, we should check that they all have
        // the same destination.
        IEdge one = c1.at(0);
        auto same = [one] (const IEdge &edge) -> bool {
            return (*(edge.get_dst()) == *(one.get_dst()));
        };
        result = std::all_of(c1.cbegin(), c2.cend(), same);
        result = result && std::all_of(c2.cbegin(), c2.cend(), same);
    }
    return (result);
}


} //
