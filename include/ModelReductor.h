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

    //helpers to infer arrays data
    void compute_int(int &to, shared_ptr<Exp> size);

    //reduce array expressions
    void reduce_size(shared_ptr<ArrayDecl> decl);
    void reduce_range(shared_ptr<Ranged> decl);
    void reduce_init(shared_ptr<Initialized> decl);
    void reduce_multiple_init(shared_ptr<MultipleInitialized> decl);
    void check_data(const ArrayData &data);


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
    //void visit(shared_ptr<ArrayPosition>);
};

#endif
