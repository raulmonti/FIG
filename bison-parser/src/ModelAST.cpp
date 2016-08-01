#include "ModelAST.h"

using namespace ASTNode;

void ModelAST::accept(Visitor &visit) {
    visit.visit(this);
    cout << "::::" << endl;
}

//Model
Model::~Model() {
    for (auto &entry : modules) {
	delete entry.second;
    }
    for (auto *decl : globals) {
	delete decl;
    }
}

void Model::accept(Visitor& visitor) {
  visitor.visit(this);
}

//Module Body
ModuleBody::~ModuleBody() {
    for (auto *decl : local_decls) {
	delete decl;
    }
    for (auto *action : actions) {
	delete action;
    }
}

void ModuleBody::accept(Visitor& visitor) {
  visitor.visit(this);
}

//Decl
Decl::~Decl() {
    for (auto *exp : inits) {
	delete exp;
    }
    delete lower;
    delete upper;
    delete size;
}

void Decl::accept(Visitor& visitor) {
  visitor.visit(this);
}

//Action
Action::~Action() {
    for (auto *effect : effects) {
	delete effect;
    }
    delete clock_loc;
    delete guard;
}

void Action::accept(Visitor& visitor) {
  visitor.visit(this);
}

//Effect
Effect::~Effect() {
    delete loc;
    delete dist;
    delete arg;
}

void Effect::accept(Visitor& visitor) {
  visitor.visit(this);
}

//Dist
Dist::~Dist() {
    delete param1;
    delete param2;
}

void Dist::accept(Visitor& visitor) {
  visitor.visit(this);
}

//Location
Location::~Location() {
    delete index;
}

void Location::accept(Visitor& visitor) {
  visitor.visit(this);
}

//Exp
void Exp::accept(Visitor& visitor) {
  visitor.visit(this);
}

//LocExp
void LocExp::accept(Visitor& visitor) {
  visitor.visit(this);
}

//IConst
void IConst::accept(Visitor& visitor) {
  visitor.visit(this);
}

//BConst
void BConst::accept(Visitor& visitor) {
  visitor.visit(this);
}

//OpExp
void OpExp::accept(Visitor& visitor) {
  visitor.visit(this);
}

//FConst
void FConst::accept(Visitor& visitor) {
  visitor.visit(this);
}
