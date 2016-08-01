#ifndef MODEL_PRINTER_H
#define MODEL_PRINTER_H
#include <iostream>

#include "ModelAST.h"

using namespace std;
using namespace ASTNode;

class ModelPrinter : public Visitor {
    int ident = 0;
    void print_idented(string str);
    void accept_idented(ModelAST *node);
 public:
    ModelPrinter() : ident {0} {};
    
    void visit(ModelAST* node);
    void visit(Model* node);
    void visit(ModuleBody* node);
    void visit(Decl* node);
    void visit(Action* node);
    void visit(Effect* node);
    void visit(Dist* node);
    void visit(Location* node);
    void visit(Exp* node);
    void visit(IConst* node);
    void visit(BConst* node);
    void visit(FConst* node);
    void visit(LocExp* node);
    void visit(OpExp* node);
};

#endif
