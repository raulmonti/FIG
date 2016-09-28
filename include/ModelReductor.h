#ifndef MODEL_REDUCTOR_H
#define MODEL_REDUCTOR_H

#include "ModelAST.h"
#include "ModuleScope.h"
#include "Operators.h"

class ModelReductor : public Visitor {
private:
    shared_ptr<ModuleScope> current_scope = nullptr;
    shared_ptr<Exp> reduce(shared_ptr<Exp> node);
    void reduce_vector(shared_vector<Exp> &vector);

public:
    ModelReductor() {}

    void accept_cond(shared_ptr<ModelAST>);

    void visit(shared_ptr<Model>);
    void visit(shared_ptr<ModuleAST>);
    void visit(shared_ptr<TransientProp>);
    void visit(shared_ptr<RateProp>);

    // Declarations
    void visit(shared_ptr<InitializedDecl>);
    void visit(shared_ptr<RangedDecl>);
    void visit(shared_ptr<ClockDecl>);
    void visit(shared_ptr<ArrayDecl>);
    void visit(shared_ptr<InitializedArray>);
    void visit(shared_ptr<MultipleInitializedArray>);
    void visit(shared_ptr<RangedInitializedArray>);
    void visit(shared_ptr<RangedMultipleInitializedArray>);

    //Transitions
    void visit(shared_ptr<TransitionAST>);

    //Effects
    void visit(shared_ptr<Assignment>);
    void visit(shared_ptr<ClockReset>);

    //Distributions
    void visit(shared_ptr<SingleParameterDist>);
    void visit(shared_ptr<MultipleParameterDist>);

    //Locations
    void visit(shared_ptr<ArrayPosition>);
};

#endif
