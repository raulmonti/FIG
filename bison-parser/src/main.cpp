#include "ModelBuilder.h"
#include "ModelPrinter.h"
#include "ModelTC.h"

using std::cout;
using std::cerr;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "Expect filename!" << endl;
    exit(1);
  }
  Log log;
  ModelBuilder builder(&log);
  string filename = argv[1];
  Model *model = builder.build(filename);
  if (log.has_errors()) {
      cout << log.get_msg();
  } else {
      ModelPrinter printer;
      model->accept(printer);
      ModelTC typechecker(&log);
      model->accept(typechecker);
      if (log.has_errors()) {
	  cout << log.get_msg();
      }
  }
}






