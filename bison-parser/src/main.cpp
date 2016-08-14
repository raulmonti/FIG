#include <memory>

#include "ModelPrinter.h"
#include "ModelTC.h"
#include "ModelBuilder.h"
#include "Util.h"

using std::cout;
using std::cerr;
using std::shared_ptr;

int main(int argc, char *argv[]) {
    if (argc < 2) {
	cerr << "Expect filename!" << endl;
	exit(1);
    }
    char *model_file = argv[1];
    char *prop_file = nullptr;
    if (argc == 3) {
	prop_file = argv[2];
    }
    shared_ptr<ModelAST> model = ModelAST::from_files(model_file, prop_file);
    if (model == nullptr) {
       	cerr << "Couldn't parse model." << endl;
	return 1;
    }
    
    ModelPrinter printer;
    model->accept(printer);
    ModelTC typechecker;
    model->accept(typechecker);
    if (typechecker.has_errors()) {
	std::cerr << typechecker.get_errors();
    }
    else {
        std::cout << "Typechecked OK" << std::endl;
	ModelBuilder builder;
	model->accept(builder);
	if (builder.has_errors()) {
	    std::cerr << builder.get_errors();
	}
    }
}






