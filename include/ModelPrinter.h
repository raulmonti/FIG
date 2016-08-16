/* Leonardo Rodríguez */

#ifndef MODEL_PRINTER_H
#define MODEL_PRINTER_H
#include <iostream>

#include "ModelAST.h"

/** Visitor to print the Model's AST **/

using std::cout;
using std::shared_ptr;

class ModelPrinter : public Visitor {
    int ident = 0;
    void print_idented(string str);
    void accept_idented(shared_ptr<ModelAST> node);
 public:
    ModelPrinter() : ident {0} {};
    virtual ~ModelPrinter() {};
    
    void visit(shared_ptr<Model> node);
    void visit(shared_ptr<ModuleBody> node);
    void visit(shared_ptr<Decl> node);
    void visit(shared_ptr<Action> node);
    void visit(shared_ptr<Effect> node);
    void visit(shared_ptr<Dist> node);
    void visit(shared_ptr<Location> node);
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
    void visit(shared_ptr<Prop> node);

    static string to_str(Type type);
    static string to_str(LabelType type);
    static string to_str(DistType type);
    static string to_str(PropType type);
    static string to_str(ExpOp type);
};

#endif