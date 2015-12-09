#ifndef PARSING_CONTEXT_H
#define PARSING_CONTEXT_H

#include <vector>
#include <string>
#include <map>
#include <set>
#include <tuple>
#include "iosacompliance.h"

using namespace std;

namespace parser{




class parsingContext{

    // Map from module name to variable name to type:
    map<string, map<string,int>> typeMap;
    // Map from modules to clock names:
    map<string,set<string>> clckMap;

public:

    parsingContext(void);

    parsingContext(const parsingContext & pc);

    virtual ~parsingContext(void);

    /*
    */
    bool
    add_var(string module, string name, int t);

    /*
    */
    bool
    add_clock(string module, string c);

    /*
    */
    bool
    has_var(string module, string v);

    /*
    */
    int
    get_var_type(string module, string name);

    /*
    */
    bool
    has_clock(string module, string c);

    /*
    */
    vector<pair<string,int>>
    get_type_list(string module);

};

}

#endif // PARSING_CONTEXT_H
