#ifndef PARSING_CONTEXT_H
#define PARSING_CONTEXT_H

#include <vector>
#include <string>
#include <map>
#include <set>

using namespace std;

namespace parser{


/**/
typedef enum    { mARIT
                , mBOOL
                , mNOTYPE
                } Type; 



class parsingContext{

    // Map from module name to variable name to type:
    map<string, map<string,Type>> typeMap;
    // Map from modules to clock names:
    map<string,set<string>> clckMap;

public:

    parsingContext(void);
    virtual ~parsingContext(void);

    /*
    */
    bool
    add_var(string module, string name, Type t);

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
    parser::Type
    get_var_type(string module, string name);

    /*
    */
    bool
    has_clock(string module, string c);

};

}

#endif // PARSING_CONTEXT_H
