/* Leonardo Rodríguez */

#ifndef CONFLUENCE_CHECKER_H
#define CONFLUENCE_CHECKER_H

#include <set>

#include "IOSAModule.h"

/**
 * A visitor to check the confluence of committed actions
 * in the model.
 *
 * @see Monti, D'Argentio: IOSA with committed actions.
 * @todo: Support for arrays.
 */

namespace iosa {

class ConfluenceChecker : public Visitor {

    /// Pairs of non-confluent actions on any of the modules.
    std::vector<NonConfluentPair> non_confluents;

    /// Triggering relation
    std::vector<TriggeringPair> tr;

    /// Initially enabled actions.
    IEdgeSet initials;

    /// Spontaneously enabled actions.
    IEdgeSet spontaneous;

    /// Maps each label to its position on the matrix
    std::map<string, unsigned int> position;

    /// We use this matrix to implement the Warshall algorithm that
    /// computes the reflexive transivie closure of the triggering
    /// relation.
    std::vector<std::vector<bool>> matrix;

public:

    /// Runs the algorithm to the given model.
    void visit(std::shared_ptr<Model> node);

private:

    /// Prints debug info
    void print_debug_info();

    /// Initialize matrix with the obtained triggering relation.
    void prepare_matrix();

    /// Run Warshall algorithm
    void warshall();

    /// Prepare error messages in case the algorithm finds non-determism
    void initial_non_deterministic_msg(NonConfluentPair &pair, IEdge &edge1, IEdge &edge2);
    void spontaneous_non_deterministic_msg(NonConfluentPair &pair, IEdge &edge);

    /// Run the actual algorithm
    bool confluence_check();

    /// Check if the given labels are on the
    bool indirectly_triggers(const string &label1, const string &label2);

    /// Print debug information
    void debug_matrix();
};

} //

#endif
