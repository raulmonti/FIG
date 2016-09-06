/* Leonardo Rodríguez */
#include "ModelAST.h"
#include "ModelParser.hpp"
#include "ErrorMessage.h"
#include <FigLog.h>
#include <cstdlib>
#include <cstring>

using std::shared_ptr;
using std::static_pointer_cast;
using fig::figTechLog;

void ModelAST::accept(Visitor &visit) {
    visit.visit(shared_from_this());
}

shared_ptr<ModelAST> ModelAST::from_files(const char *model_file,
										  const char *prop_file)
{
	shared_ptr<ModelAST> result = nullptr;
	ModelParserGen::ModelParser parser {&result};
	int res(1);
	// Process model file
	FILE* file = std::fopen(model_file, "r");
	if (nullptr == file) {
		figTechLog << "Model file \"" << model_file << "\" does not exists!\n";
		res = 1;
		goto exit_point;
	}
	scan_begin(file);
	res = parser.parse();
	scan_end();
	if (0 != res || nullptr == result) {
		figTechLog << "Errors found while parsing \"" << model_file << "\"\n";
		goto exit_point;
	}
	// Process properties file, if any
	if (nullptr != prop_file && 0ul < strnlen(prop_file, 128ul)) {
		file = fopen(prop_file, "r");
		if (nullptr == file) {
			figTechLog << "Properties file \"" << model_file
							<< "\" does not exists!\n";
			res = 1;
			goto exit_point;
		}
		scan_begin(file);
		res = parser.parse();
		scan_end();
		if (0 != res) {
			figTechLog << "Errors found while parsing \"" << prop_file << "\"\n";
			goto exit_point;
		}
	}
	exit_point:
		if (0 <= fileno(file))
			std::fclose(file);
		return (res == 0 ? result : nullptr);
}

void ModelAST::on_scanner_error(const string &msg) {
	figTechLog << "Syntax error: " << msg << std::endl;
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
