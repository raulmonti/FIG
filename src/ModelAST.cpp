/* Leonardo Rodríguez */
#include "ModelAST.h"
#include "ModelParser.hpp"
#include "ErrorMessage.h"
#include <cstdlib>

using std::shared_ptr;
using std::static_pointer_cast;

void ModelAST::accept(Visitor &visit) {
    visit.visit(shared_from_this());
}

shared_ptr<ModelAST> ModelAST::from_files(const char *model_file,
                                          const char *prop_file) {
    shared_ptr<ModelAST> result = nullptr;
    ModelParserGen::ModelParser parser {&result};
    FILE *file = fopen(model_file, "r");
    if (file == nullptr) {
        std::cerr << "Model file does not exists!" << std::endl;
        exit(1);
    }
    scan_begin(file);
    int res = parser.parse();
    scan_end();
    if (result != nullptr && prop_file != nullptr) {
        //try to read more properties from the properties file.
        file = fopen(prop_file, "r");
        if (file != nullptr) {
            scan_begin(file);
            res = parser.parse();
            scan_end();
        }
    }
    return (res == 0 ? result : nullptr);
}

void ModelAST::on_scanner_error(const string &msg) {
    std::cerr << "Syntax error: " << msg << std::endl;
}

void Model::accept(Visitor& visit) {
    //my_type == Model
    using my_type = std::remove_pointer<decltype(this)>::type;
    const auto &_this = static_pointer_cast<my_type>(shared_from_this());
    visit.visit(_this);
}

void ModuleBody::accept(Visitor& visit) {
    using my_type = std::remove_pointer<decltype(this)>::type;
    visit.visit(static_pointer_cast<my_type>(shared_from_this()));
}

void Decl::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Decl>(shared_from_this()));
}

void Action::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Action>(shared_from_this()));
}

void Effect::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Effect>(shared_from_this()));
}

void Dist::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Dist>(shared_from_this()));
}

void Location::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Location>(shared_from_this()));
}

void Prop::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Prop>(shared_from_this()));
}

//Exp
void Exp::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Exp>(shared_from_this()));
}

//LocExp
void LocExp::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<LocExp>(shared_from_this()));
}

//IConst
void IConst::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<IConst>(shared_from_this()));
}

//BConst
void BConst::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<BConst>(shared_from_this()));
}

//OpExp
void OpExp::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<OpExp>(shared_from_this()));
}

//FConst
void FConst::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<FConst>(shared_from_this()));
}

//Default Visitor does nothing on his visitation ;)
Visitor::Visitor() {
    message = make_shared<ErrorMessage>();
}

void Visitor::visit(shared_ptr<ModelAST> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<Model> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<ModuleBody> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<Decl> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<Action> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<Effect> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<Dist> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<Location> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<Exp> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<IConst> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<BConst> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<FConst> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<LocExp> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<OpExp> node) {
    (void) node;
}

void Visitor::visit(shared_ptr<Prop> node) {
    (void) node;
}

void Visitor::put_error(const string &msg) {
    message->put_error(msg);
}

void Visitor::put_warning(const string &msg) {
    message->put_warning(msg);
}

bool Visitor::has_errors() {
    return (message->has_errors());
}

bool Visitor::has_warnings() {
    return (message->has_warnings());
}

string Visitor::get_messages() {
    return (message->get_msg());
}
