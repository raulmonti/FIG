#include "ModelPrinter.h"
#include "ModelTC.h"
#include "Util.h"

using std::cout;
using std::cerr;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "Expect filename!" << endl;
    exit(1);
  }
  Log &log = Log::get_instance();
  string filename = argv[1];
  ModelAST *model = ModelAST::from_file(filename);
  ModelPrinter printer;
  model->accept(printer);
  ModelTC typechecker;
  model->accept(typechecker);
  if (log.has_errors()) {
      std::cerr << log.get_msg();
  } else {
      std::cout << "Typechecked OK" << std::endl;
  }

  delete model;
}






