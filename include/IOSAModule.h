/* Leonardo Rodríguez */
#ifndef IOSAMODULE_H
#define IOSAMODULE_H

#include "Graph.h"
#include "IOSAState.h"
#include "ModelAST.h"
#include "ModuleScope.h"

namespace iosa {


using IVert = shared_ptr<State>;

class TransitionInfo {
private:
    std::string label_id;
    LabelType type;

public:
    TransitionInfo(const std::string& label_id, LabelType type):
        label_id {label_id}, type {type} {}

    std::string get_label_id() const {
        return label_id;
    }

    LabelType get_label_type() const {
        return type;
    }

    bool operator==(const TransitionInfo& that) const {
        return (label_id == that.get_label_id())
                && (type == that.get_label_type());
    }
};

using IEdge = Edge<shared_ptr<State>, TransitionInfo>;
using NonConfluentPair = std::pair<IEdge, IEdge>;
using TriggeringPair = std::pair<IEdge, IEdge>;
using IEdgeSet = std::vector<IEdge>;

struct StatePtrComp {
    bool operator()(const IVert &s1, const IVert &s2) const {
        return (*s1 < *s2);
    }
};

class ModuleIOSA : Graph <shared_ptr<State>, TransitionInfo, StatePtrComp> {
private:
    IVert initial_state;
    std::shared_ptr<ModuleScope> scope;
    std::shared_ptr<ModuleAST> ast;

public:

    ModuleIOSA(std::shared_ptr<ModuleAST> ast);
    void search_non_confluents(std::vector<NonConfluentPair> &result);
    void search_triggering_pairs(std::vector<TriggeringPair> &result);
    void search_initially_enabled(IEdgeSet &edges);
    void search_spontaneous(IEdgeSet &result);

private:

    int get_value(shared_ptr<Exp> exp) const;
    void add_variable(shared_ptr<Decl> decl);
    void build_initial_state();
    void process_transitions();
    bool holds_expression(IVert st, shared_ptr<Exp> bexp) const;
    IVert process_edge(IVert st, shared_ptr<TransitionAST> transition);
    IVert process_assignments(IVert st, shared_vector<Assignment>& avec);
    IEdgeSet select_edges_of(IVert src, std::function<bool (const IEdge &)> prop);
    IEdgeSet committed_edges_of(IVert st);
    IEdgeSet labeled_edges_of(IVert st, const string &label);
    void non_confluents_of(std::vector<NonConfluentPair> &result, IVert st);
    bool edge_confluent(IEdge &edge1, IEdge &edge2);
    void triggering_pairs_on(IEdgeSet &edges, std::vector<TriggeringPair> &result);
    IEdgeSet reachable_edges_of(IVert st);
    IEdgeSet edges_of(IVert st);
    void spontaneous_on(IEdgeSet &edges, IEdgeSet &result);
    void insert(IEdgeSet &set, const IEdge &edge);
};

} //

#endif

