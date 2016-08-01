#include "ModelBuilder.h"
#include "ModelPrinter.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "Expect filename!" << endl;
    exit(1);
  }
  ModelBuilder builder;
  string filename = argv[1];
  Model *model = builder.build(filename);
  ModelPrinter printer;
  model->accept(printer);
}






