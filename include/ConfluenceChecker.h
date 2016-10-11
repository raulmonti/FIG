/* Leonardo Rodríguez */

#ifndef CONFLUENCE_CHECKER_H
#define CONFLUENCE_CHECKER_H

#include <set>

#include "IOSAModule.h"

namespace iosa {

class ConfluenceChecker : public Visitor {
    std::vector<NonConfluentPair> non_confluents;
    std::vector<TriggeringPair> tr;
    IEdgeSet initials;
    IEdgeSet spontaneous;

    //transitive closure stuff.
    std::map<string, unsigned int> position;
    std::vector<std::vector<bool>> matrix;

public:
    void visit(std::shared_ptr<Model> node);
private:
    void print_debug_info();
    void prepare_matrix();
    void warshall();
    void initial_non_deterministic_msg(NonConfluentPair &pair, IEdge &edge1, IEdge &edge2);
    void spontaneous_non_deterministic_msg(NonConfluentPair &pair, IEdge &edge);
    bool confluence_check();
    bool indirectly_triggers(const string &label1, const string &label2);
    void debug_matrix();
};

} //

#endif
