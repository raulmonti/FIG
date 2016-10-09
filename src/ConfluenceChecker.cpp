/* Leonardo Rodríguez */

#include "ConfluenceChecker.h"

namespace iosa {

void ConfluenceChecker::visit(std::shared_ptr<Model> node) {
    for (auto module : node->get_modules()) {
        ModuleIOSA iosa(module);
    }
}

} //
