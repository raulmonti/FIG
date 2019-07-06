/* Leonardo Rodríguez */

#ifndef IOSAMODULE_H
#define IOSAMODULE_H

#include "Graph.h"
#include "IOSAState.h"
#include "ModelAST.h"
#include "ModuleScope.h"

namespace iosa {

/**
 * Here we define some classes used to build an explicit automata
 * that is necessary to check the confluence of a model with committed
 * actions @see \ref ConfluenceChecker
 */

/// The vertex of the underlying graph of the automata will be an state.
using IVert = shared_ptr<State>;

/**
 * @brief Each edge of the graph holds a label an type (input, output, committed)
 */
class TransitionInfo {
private:
    /// Label associated with the transition
    std::string label_id;
    /// Type of the transition (!, ?, !!, ??)
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

/// An edge joining two states.
using IEdge = Edge<shared_ptr<State>, TransitionInfo>;

/// A non-confluent pair of edges
/// @see Definition in Monti-D'Argenio paper.
using NonConfluentPair = std::pair<IEdge, IEdge>;

/// @see Definition of "triggering pair" in Monti-D'Argenio paper
using TriggeringPair = std::pair<IEdge, IEdge>;

/// A "set" of edges
using IEdgeSet = std::vector<IEdge>;

struct StatePtrComp {
    bool operator()(const IVert &s1, const IVert &s2) const {
        return (*s1 < *s2);
    }
};

/** @brief This class represents an explicit IOSA, a graph in which every
 * vertex is an state and every edge is a transition.
 * @see D'Argentio-Monti : "IOSA with committed actions". That paper contains the
 * pseudocodes of the algorithms implemented here.
*/
class ModuleIOSA : Graph <shared_ptr<State>, TransitionInfo, StatePtrComp> {
private:
    /// The initial state of the automata
    IVert initial_state;
    /// When this IOSA is a single module, we keep that module scope.
    std::shared_ptr<ModuleScope> scope;
    /// The AST that generate the module associated with this IOSA
    std::shared_ptr<ModuleAST> ast;

public:

    ModuleIOSA(std::shared_ptr<ModuleAST> ast);

    /// BFS the automata looking for pairs of non-confluent committed transitions
    void search_non_confluents(std::vector<NonConfluentPair> &result);

    /// BFS the automata looking for triggering pairs.
    void search_triggering_pairs(std::vector<TriggeringPair> &result);

    /// Edges enabled by the initial state.
    void search_initially_enabled(IEdgeSet &edges);

    /// BFS the automata looking for spontaneous edges
    void search_spontaneous(IEdgeSet &result);

private:
    // Some auxiliary functions to implement the algoritmhs.
    int get_value(shared_ptr<Exp> exp) const;
    void add_variable(shared_ptr<Decl> decl);
    void build_initial_state();
    void process_transitions();
    bool holds_expression(IVert st, shared_ptr<Exp> bexp) const;
    IVert process_edge(IVert st, shared_ptr<TransitionAST> transition);
	IVert process_assignments(IVert st, const shared_vector<Assignment>& avec);
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

