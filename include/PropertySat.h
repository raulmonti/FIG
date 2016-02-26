#ifndef PROPERTY_SAT_H
#define PROPERTY_SAT_H

#include <string>
#include <vector>

#include <z3++.h>

#include "core_typedefs.h"


using namespace std;

namespace fig{


class PropertySat{

private:

    z3::context      c_;
    vector<z3::expr> propExpr_;
    z3::expr         limitsExpr_;
    vector<string>   vNames_;

public:

    /**
     * @brief Build a satisfiability checker for the @idx parsed property. FIXME
     */
    PropertySat(unsigned int idx, vector<string> &vnames);

    virtual ~PropertySat(){};

    /**
     * @brief Deduce if the @idx boolean formula of this property is
     *        satisfiable given valuation @valuation. FIXME
     */
    bool
    sat(unsigned int idx, vector<STATE_INTERNAL_TYPE> valuation);
};

} //namespace fig


#endif // PROPERTY_SAT_H
