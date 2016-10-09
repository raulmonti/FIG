/* Leonardo Rodríguez */
#ifndef IOSAMODULE_H
#define IOSAMODULE_H

#include "Graph.h"
#include "IOSAState.h"
#include "ModelAST.h"
#include "ModuleScope.h"

namespace iosa {

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

using IEdge = Edge<shared_ptr<State>, TransitionInfo>;
using IVert = shared_ptr<State>;


struct StatePtrComp {
    bool operator()(const IVert &s1, const IVert &s2) const {
        return (*s1 < *s2);
    }
};

class ModuleIOSA : Graph <shared_ptr<State>, TransitionInfo, StatePtrComp> {
private:
    std::shared_ptr<State> initial_state;
    std::shared_ptr<ModuleScope> scope;
    std::shared_ptr<ModuleAST> ast;

public:

    ModuleIOSA(std::shared_ptr<ModuleAST> ast);

private:

    int get_value(shared_ptr<Exp> exp) const;
    void add_variable(shared_ptr<Decl> decl);
    void build_initial_state();
    void process_transitions();
    bool holds_expression(IVert st, shared_ptr<Exp> bexp) const;
    IVert process_edge(IVert st, shared_ptr<TransitionAST> transition);
    IVert process_assignments(IVert st, shared_vector<Assignment>& avec);
};

} //

#endif

