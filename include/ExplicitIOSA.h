#ifndef EXPLICIT_IOSA_H
#define EXPLICIT_IOSA_H

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cassert>
#include <utility>
#include <queue>
#include <functional>

#include "ModelAST.h"
#include "ModuleScope.h"

using state_value_t = int;
using state_pos_t = unsigned long int;

namespace iosa {

class FixedRange {
private:
    state_value_t min;
    state_value_t max;
public:
    FixedRange(state_value_t min, state_value_t max) :
        min {min}, max {max} {}

    state_value_t get_min() const {
        return (min);
    }

    state_value_t get_max() const {
        return (max);
    }
};

class State {
private:
    std::vector<state_value_t> values;
    std::unordered_map<std::string, state_pos_t> pos;
    std::map<std::string, FixedRange> ranges;

public:
    State() {}

    State(const State& state) = default;

    void add_variable(const std::string &name, const FixedRange& range) {
        assert(ranges.find(name) == ranges.end());
        ranges.insert(std::make_pair(name,range));
        state_pos_t i = values.size();
        pos[name] = i;
        values.push_back(range.get_min());
    }

    void set_variable_value(const std::string &name, state_value_t value) {
        assert(pos.find(name) != pos.end());
        state_pos_t i = pos.at(name);
        assert(i < values.size());
        values[i] = value;
    }

    state_value_t get_variable_value(const std::string &name) const {
        state_pos_t i = pos.at(name);
        assert(i < values.size());
        return values[i];
    }

    bool is_valid() const {
        bool result = true;
        for (auto entry : pos) {
            state_pos_t i = entry.second;
            state_value_t v = values[i];
            FixedRange range = ranges.at(entry.first);
            if (v < range.get_min() || v > range.get_max()) {
                result = false;
            }
        }
        return (result);
    }

    void print_state(std::ostream& ss);
    bool operator==(const State &that);
    bool operator!=(const State &that);
    bool operator<(const State &that);
};

class Evaluator : public Visitor {
private:
    state_value_t value;
    shared_ptr<State> state;

public:
    Evaluator(shared_ptr<State> &state)
        : value {0}, state {state} {}

    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<BinOpExp> node);
    void visit(shared_ptr<UnOpExp> node);

    state_value_t get_value() {
        return (value);
    }
};


// Maybe we should use unorderd map and hash the vector
// http://stackoverflow.com/questions/20511347/
// http://stackoverflow.com/questions/37007307/

template<typename V, typename D>
class Edge {
private:
    V src;
    V dst;
    D data;
public:
    Edge(const V& src, const V& dst, const D& data) :
        src {src}, dst {dst}, data {data} {}

    V get_src() const {
        return src;
    }

    V get_dst() const {
        return dst;
    }

    D get_data() const {
        return data;
    }
};

template<typename V, typename D, typename VComp = std::less<V>>
class Graph {
private:
    std::multimap<V, Edge<V, D>, VComp> edges;
public:
    Graph() {}

    void add_edge(const Edge<V,D> &edge) {
        const auto& e = std::make_pair(edge.get_src(), edge);
        edges.insert(e);
    }

    bool has_vertex(const V& v) const {
        return edges.find(v) != edges.cend();
    }

    bool has_edge(const Edge<V,D> &edge) {
        auto its = edges.equal_range(edge.get_src());
        auto fst = its.first;
        auto snd = its.second;
        bool result = false;
        while (fst != snd && !result) {
            const Edge<V,D>& curr = fst->second;
            result = same_edge(curr, edge);
            /*std::cout << "COMPARE: ";
            edge.get_src()->print_state(std::cout);
            std::cout << " WITH: ";
            edge.get_dst()->print_state(std::cout);
            std::cout << " RESULT : " <<  result;
            std::cout << std::endl;*/
            fst++;
        }
        return (result);
    }

    bool same_vertex(const V& v1, const V& v2) {
        auto less = edges.key_comp();
        return (!less(v1, v2) && !less(v2, v1));
    }

    bool same_edge(const Edge<V,D> &edge1, const Edge<V,D> &edge2) {
        return same_vertex(edge1.get_src(), edge2.get_src()) &&
                same_vertex(edge1.get_dst(), edge2.get_dst());
    }

    void print(std::function<void (Edge<V, D>)> printer) {
        auto it = edges.cbegin();
        while (it != edges.cend()) {
            const Edge<V,D>&edge = it->second;
            printer(edge);
            it++;
        }
    }
};

class TransitionInfo {
private:
    std::string label_id;
    LabelType type;

public:
    TransitionInfo(const std::string& label_id, LabelType type):
        label_id {label_id}, type {type} {}

    std::string get_label_id() {
        return label_id;
    }

    LabelType get_label_type() {
        return type;
    }
};

class ModuleIOSA {
private:
    std::shared_ptr<State> initial_state;
    std::shared_ptr<ModuleScope> scope;
    std::shared_ptr<ModuleAST> ast;

    struct Comp {
        bool operator()(const shared_ptr<State> &s1,
                        const shared_ptr<State> &s2) {
            return (*s1 < *s2);
        }
    };

    using IEdge = Edge<shared_ptr<State>, TransitionInfo>;

    Graph<std::shared_ptr<State>, TransitionInfo, Comp> iosa;

public:
    ModuleIOSA(std::shared_ptr<ModuleAST> ast) {
        this->ast = ast;
        this->scope = ModuleScope::scopes.at(ast->get_name());
        build_initial_state();
        process_transitions();
        auto pr  = [] (Edge<shared_ptr<State>,TransitionInfo> edge) -> void {
            TransitionInfo data = edge.get_data();
            edge.get_src()->print_state(std::cout);
            std::cout << "--[ ";
            std::cout << data.get_label_id();
            std::cout << " ]-->";
            edge.get_dst()->print_state(std::cout);
            std::cout << std::endl;
        };
        iosa.print(pr);
    }

private:

    int get_value(shared_ptr<Exp> exp) const {
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

    void add_variable(shared_ptr<Decl> decl) {
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

    void build_initial_state() {
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

    void process_transitions() {
        shared_ptr<State> next = initial_state;
        std::queue<shared_ptr<State>> states;
        states.push(initial_state);
        while (!states.empty()) {
            shared_ptr<State> current = states.front();
            states.pop();
            for (auto entry : scope->transition_by_label_map()) {
                shared_ptr<TransitionAST> tr = entry.second;
                next = add_edge(current, tr);
                if (next != nullptr) {
                    states.push(next);
                }
            }
        }
    }

    bool holds_expression(shared_ptr<State> st, shared_ptr<Exp> bexp) {
        Evaluator evaluator (st);
        bexp->accept(evaluator);
        return (evaluator.get_value() == 1);
    }

    shared_ptr<State> add_edge(shared_ptr<State> st,
                               shared_ptr<TransitionAST> transition) {
        shared_ptr<State> result = nullptr;
        shared_ptr<Exp> pre = transition->get_precondition();
        if (holds_expression(st, pre)) {
            shared_vector<Assignment>& as = transition->get_assignments();
            shared_ptr<State> cpy = process_assignments(st, as);
            if (!cpy->is_valid()) {
                throw_FigException("Generated out-of-range state.");
            }
            TransitionInfo tinfo
                    (transition->get_label(), transition->get_label_type());
            IEdge edge (st, cpy, tinfo);
            if (!iosa.has_edge(edge)) {
                result = cpy;
                iosa.add_edge(edge);
            }
        }
        return (result);
    }

    shared_ptr<State> process_assignments(shared_ptr<State> st,
                                          shared_vector<Assignment>& avec) {
        shared_ptr<State> copy = make_shared<State>(*st);
        for (shared_ptr<Assignment> as : avec) {
            shared_ptr<Exp> rhs = as->get_rhs();
            Evaluator evaluator (st);
            rhs->accept(evaluator);
            std::string name = as->get_effect_location()->get_identifier();
            copy->set_variable_value(name, evaluator.get_value());
        }
        return (copy);
    }

};

class ConfluenceVerifier : public Visitor {
public:
    void visit(std::shared_ptr<Model> node) {
        for (auto module : node->get_modules()) {
            ModuleIOSA iosa(module);
        }
    }
};

} //namespace iosa



#endif //EXPLICIT_IOSA_H
