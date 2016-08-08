#include "ModelAST.h"
#include "ModelParser.hpp"
#include <cstdlib>

void ModelAST::accept(Visitor &visit) {
    visit.visit(this);
}

ModelAST *ModelAST::from_file(const string &filename) {
    ModelAST *result = nullptr;
    ModelParserGen::ModelParser parser {&result};
    FILE *file = fopen(filename.c_str(), "r");
    if (file == nullptr) {
	std::cerr << "File does not exists!" << std::endl;
	exit(1);
    }
    scan_begin(file);
    int res = parser.parse();
    scan_end();
    return (res == 0 ? result : nullptr);
}

void ModelAST::on_scanner_error(const string &msg) {
    std::cerr << "Syntax error: " << msg << std::endl;
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

//Default Visitor does nothing on his visitation ;)

void Visitor::visit(ModelAST *node) {
    (void) node;
}

void Visitor::visit(Model *node) {
     (void) node;
}

void Visitor::visit(ModuleBody *node) {
    (void) node;
}

void Visitor::visit(Decl *node) {
    (void) node;
}

void Visitor::visit(Action *node) {
    (void) node;
}

void Visitor::visit(Effect *node) {
    (void) node;
}

void Visitor::visit(Dist *node) {
    (void) node;
}

void Visitor::visit(Location *node) {
    (void) node;
}

void Visitor::visit(Exp *node) {
    (void) node;
}

void Visitor::visit(IConst *node) {
    (void) node;
}

void Visitor::visit(BConst *node) {
    (void) node;
}

void Visitor::visit(FConst *node) {
    (void) node;
}

void Visitor::visit(LocExp *node) {
    (void) node;
}

void Visitor::visit(OpExp *node) {
    (void) node;
}
