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
    string filename = argv[1];
    shared_ptr<ModelAST> model = ModelAST::from_file(filename);
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






