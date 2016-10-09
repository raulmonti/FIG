/* Leonardo Rodríguez */

#ifndef CONFLUENCE_CHECKER_H
#define CONFLUENCE_CHECKER_H

#include "IOSAModule.h"

namespace iosa {

class ConfluenceChecker : public Visitor {
public:
    void visit(std::shared_ptr<Model> node);
};

} //

#endif
