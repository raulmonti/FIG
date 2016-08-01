#include "ModelBuilder.h"


using namespace std;
using namespace ModelParserGen;

void ModelBuilder::error(location loc, string msg) {
  cerr << "parser error in " << loc << " : " << msg << endl;
}

Model *ModelBuilder::build(const std::string& str) {
    file = fopen(str.c_str(), "r");
    ModelParser parser {*this};
    scan(file);
    parser.parse();
    scan_end();
    return (this->model);
}

ModelBuilder::~ModelBuilder() {    
}
