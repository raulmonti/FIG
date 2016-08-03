#include "ModelBuilder.h"

using namespace ModelParserGen;

void ModelBuilder::error(location loc, string msg) {
    stringstream ss;
    ss << "Syntax error on " << loc  << " : " << msg;
    log->put_error(ss.str());
}

Model *ModelBuilder::build(const std::string& str) {
    file = fopen(str.c_str(), "r");
    ModelParser parser {*this};
    scan(file);
    parser.parse();
    scan_end();
    if (log->has_errors()) {
	this->model = nullptr;
    }
    return (this->model);
}

ModelBuilder::~ModelBuilder() {    
}
