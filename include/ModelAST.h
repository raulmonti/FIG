/* Leonardo Rodríguez */

#ifndef __MODEL_AST_H__
#define __MODEL_AST_H__
#include <cassert>
#include <map>
#include <memory>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "Util.h"

/** Model Abstract Syntax Tree  **/

using std::string;
using std::vector;
using std::map;
using std::shared_ptr;
using std::make_shared;

enum class Type {tint, tbool, tfloat, tclock, tunknown};
enum class ExpOp {plus, times, minus, div, mod, andd, orr, nott, eq, neq, lt, gt, le, ge};
enum class Arity  {one, two};
enum class LabelType {in, out, commited, empty};
enum class DistType {erlang, normal, uniform, exponential};
enum class PropType {transient, rate};

// forward declare this classes to declare them friends
// (cannot include ModelParser.hpp since its generation depends on this file)
namespace ModelParserGen {
class ModelParser;
class ModelScanner;
}

class ModelAST : public std::enable_shared_from_this<ModelAST> {
protected:
private:
    friend class ModelParserGen::ModelParser;
    friend class ModelParserGen::ModelScanner;
    //starts lexer on the given file
    static void scan_begin(FILE *file);
    //finish lexer
    static void scan_end();
    static void on_scanner_error(const std::string &msg);
public:
    static shared_ptr<ModelAST> from_files(const char *model_file,
                                           const char *prop_file);
    virtual ~ModelAST() {};
    virtual void accept(class Visitor& visit);
};

class Prop : public ModelAST {
public:
    PropType type;
    shared_ptr<class Exp> left;
    shared_ptr<class Exp> right;
    
    Prop(shared_ptr<Exp> rate)
        : type {PropType::rate}, left {rate}, right {nullptr} {};
    Prop(shared_ptr<Exp> left, shared_ptr<Exp> right)
        : type {PropType::transient}, left {left}, right {right} {};
    void accept(Visitor& visit) override;
};

class Model : public ModelAST {
public:
    // public access allows the use of VisitorPattern
    shared_vector<class ModuleBody> modules;
    vector<string> modules_ids;
    shared_vector<class Decl> globals;
    shared_vector<Prop> props;
    
    Model(string id, shared_ptr<ModuleBody> mb) {
        add_module(id, mb);
    }
    
    Model(shared_ptr<Decl> decl) {
        add_decl(decl);
    }
    
    Model(const Model &model) = delete;
    void operator=(Model const &) = delete;
    
    void add_module(string id, shared_ptr<ModuleBody> mb) {
        modules.push_back(mb);
        modules_ids.push_back(id);
    }
    
    void add_decl(shared_ptr<Decl> decl) {
        globals.push_back(decl);
    }

    void add_props(const shared_vector<Prop> &properties) {
        props.insert(props.end(), properties.begin(), properties.end());
    }
    
    bool has_module(const string& id) {
        return (std::find(modules_ids.begin(), modules_ids.end(), id)
                != modules_ids.end());
    }
    
    const shared_vector<ModuleBody> &get_modules() const {
        return (modules);
    }

    const vector<string> &get_modules_ids() const {
        return (modules_ids);
    }
    
    const vector<shared_ptr<class Decl>>& get_globals() const {
        return (globals);
    }

    const vector<shared_ptr<Prop>>& get_props() const {
        return (props);
    }

    bool has_props() {
        return (props.size() > 0);
    }
    
    void accept(Visitor& visit) override;
};

class ModuleBody : public ModelAST {
public:
    shared_vector<Decl> local_decls;
    shared_vector<class Action> actions;
    
    ModuleBody() {};
    
    ModuleBody(shared_ptr<Decl> decl) {
        add_decl(decl);
    }
    
    ModuleBody(shared_ptr<Action> action) {
        add_action(action);
    }
    
    ModuleBody(const ModuleBody &) = delete;
    void operator=(const ModuleBody &) = delete;
    
    void add_decl(shared_ptr<Decl> decl) {
        local_decls.push_back(decl);
    }
    
    void add_action(shared_ptr<Action> action) {
        actions.push_back(action);
    }
    
    void accept(Visitor& visit) override;
    
    const shared_vector<Decl>& get_local_decls() {
        return local_decls;
    }
    
    const shared_vector<Action>& get_actions() {
        return actions;
    }
};

class Decl : public ModelAST {
public:
    Type type;
    string id;
    shared_vector<class Exp> inits;
    shared_ptr<Exp> lower;
    shared_ptr<Exp> upper;
    shared_ptr<Exp> size;
    
    //declaration with type, id, range, no initialization
    Decl(Type type, string id, shared_ptr<Exp> lower, shared_ptr<Exp> upper)
        : type {type}, id {id},
          lower {lower}, upper {upper},size {nullptr}
    {
        //choose lower limit as initialization
        //note: this turns the AST into an ASGraph,
        //safe only by using shared_ptr or a well-designed deleter.
        inits = vector<shared_ptr<Exp>>{lower};
    }
    
    //declaration with type, id, range and initialization
    Decl(Type type, string id, shared_ptr<Exp> lower,
         shared_ptr<Exp> upper, shared_ptr<Exp> init)
        : type {type}, id {id}, lower {lower}, upper {upper},
          size {nullptr}
    { inits = vector<shared_ptr<Exp>>{init}; }

    //declaration with type, id, initialization (no range)
    Decl(Type type, string id, shared_ptr<Exp> init)
        : type {type}, id {id},
          lower {nullptr}, upper {nullptr},
          size {nullptr}
    { inits = vector<shared_ptr<Exp>>{init};}

    //declaration with type, id, no range, no initialization
    //(this must be a clock)
    Decl(Type type, string id)
        : type {type}, id {id},
          lower {nullptr}, upper {nullptr},
          size {nullptr}
    { inits = vector<shared_ptr<Exp>>{}; }

    //declaration of array with id, size of array, range of values, initalizations.
    Decl(Type type, string id, shared_ptr<Exp> size,
         shared_ptr<Exp> lower, shared_ptr<Exp> upper,
         const shared_vector<Exp> &inits)
        : type {type}, id {id}, inits {inits},
          lower {lower}, upper {upper},
          size {size}
    {};

    //declaration of array with id, size of array, no range of values, initializaiton
    Decl(Type type, string id, shared_ptr<Exp> size,
         const shared_vector<Exp> &inits)
        : type {type}, id {id}, inits {inits},
          lower {nullptr}, upper {nullptr},
          size {size}
    {};

    Decl(const Decl &Decl) = delete;
    void operator=(const Decl &Decl) = delete;

    bool has_range() {
        return (lower != nullptr);
    }

    bool is_array() {
        return (size != nullptr);
    }

    bool has_single_init() {
        return (inits.size() == 1);
    }

    bool has_array_init() {
        return (inits.size() > 1);
    }

    const shared_vector<Exp>& get_inits() {
        return (inits);
    }

    void accept(Visitor& visit) override;
};

class Action : public ModelAST {
public:
    string id;
    LabelType type;
    shared_ptr<Exp> guard;
    shared_ptr<class Location> clock_loc;
    shared_vector<class Effect> effects;

    // e.g  [id ? ] guard -> (effect1 & effect2 & .... )
    Action(string id, LabelType type, shared_ptr<Exp> guard,
           const shared_vector<Effect> &effects)
        : id {id}, type {type}, guard {guard},
          clock_loc {nullptr}, effects {effects} {};

    // e.g [id ! ] guard @ clock -> (effect1 & effect2 & .... )
    Action(string id, LabelType type, shared_ptr<Exp> guard,
           shared_ptr<Location> clock_loc, const shared_vector<Effect> &effects)
        : id {id}, type {type}, guard {guard},
          clock_loc {clock_loc}, effects {effects} {};

    // e.g [id ! ] guard @ clock -> (effect1 & effect2 & .... )
    Action(LabelType type, shared_ptr<Exp> guard,
           shared_ptr<Location> clock_loc, const shared_vector<Effect> &effects)
        : id {""}, type {type}, guard {guard},
          clock_loc {clock_loc}, effects {effects} {};
    
    Action(const Action &Decl) = delete;
    void operator=(const Action &Decl) = delete;

    void accept(Visitor& visit) override;

    bool has_clock() {
        return (clock_loc != nullptr);
    }

    const shared_vector<Effect>& get_effects() {
        return (effects);
    }
};

class Effect : public ModelAST {
public:
    shared_ptr<Location> loc;
    shared_ptr<class Dist> dist;
    shared_ptr<Exp> arg;

    //e.g: q' = 4 + q
    Effect(shared_ptr<Location> loc, shared_ptr<Exp> arg)
        : loc {loc}, dist {nullptr}, arg {arg} {};

    //e.g: c = uniform(4, 5)
    Effect(shared_ptr<Location> loc, shared_ptr<Dist> dist)
        : loc {loc}, dist {dist}, arg {nullptr} {};
    
    Effect(const Effect &effect) = delete;
    void operator=(const Effect &effect) = delete;

    bool is_clock_reset() const {
        return (dist != nullptr);
    }

    bool is_state_change() const {
        return (arg != nullptr);
    }

    void accept(Visitor& visit) override;
};

class Dist : public ModelAST {
public:
    DistType type;
    Arity arity;
    shared_ptr<Exp> param1;
    shared_ptr<Exp> param2;

    //distribution type, arity (1 or 2), parameters.
    Dist(DistType dist_type, Arity arity,
         shared_ptr<Exp> param1, shared_ptr<Exp> param2 = nullptr) :
        type {dist_type}, arity {arity},
        param1 {param1}, param2 {param2} {};
    
    Dist(const Dist &) = delete;
    void operator=(const Dist &) = delete;

    void accept(Visitor& visit) override;
};

class Location : public ModelAST {
public:
    string id;
    shared_ptr<Exp> index;

    // id or id[exp]
    Location(string id, shared_ptr<Exp> index = nullptr) : id {id}, index {index} {};
    
    Location(const Location &) = delete;
    void operator=(const Location &) = delete;

    bool is_array_position() {
        return (index != nullptr);
    }

    void accept(Visitor& visit) override;
};

class Exp : public ModelAST {
public:
    Type type;
    Exp() : type{Type::tunknown} {};
    Exp(const Exp &) = delete;
    void operator=(const Exp &) = delete;

    void accept(Visitor& visit) override;
};

class IConst : public Exp {
public:
    int value;
    IConst(int value) : value {value} {};
    IConst(const IConst &) = delete;
    void operator=(const IConst &) = delete;

    void accept(Visitor& visit) override;
};

class BConst : public Exp {
public:
    bool value;
    BConst(bool value) : value {value} {};
    BConst(const BConst &) = delete;
    void operator=(const BConst &) = delete;

    void accept(Visitor& visit) override;
};

class FConst : public Exp {
public:
    float value;
    FConst(float value) : value {value} {};
    FConst(const FConst &) = delete;
    void operator=(const FConst &) = delete;

    void accept(Visitor& visit) override;
};

class LocExp : public Exp {
public:
    shared_ptr<Location> location;
    LocExp(shared_ptr<Location> location) : location {location} {};

    LocExp(const LocExp &) = delete;
    void operator=(const LocExp &) = delete;

    void accept(Visitor& visit) override;
};

class OpExp : public Exp {
public:
    Arity arity;
    ExpOp bop;
    shared_ptr<Exp> left;
    shared_ptr<Exp> right;

    //arity (1 or 2), operator, left argument, right argument (optional)
    OpExp(Arity arity, ExpOp bop, shared_ptr<Exp> left,
          shared_ptr<Exp> right = nullptr) :
        arity {arity}, bop {bop}, left {left}, right {right} {};

    OpExp(const OpExp &) = delete;
    void operator=(const OpExp &) = delete;

    void accept(Visitor& visit) override;

    static shared_ptr<Exp> make_nott(shared_ptr<Exp> exp) {
        return make_shared<OpExp>(Arity::one, ExpOp::nott, exp);
    }

    static shared_ptr<Exp> make_andd(shared_ptr<Exp> exp1,
                                     shared_ptr<Exp> exp2) {
        return make_shared<OpExp>(Arity::two, ExpOp::andd, exp1, exp2);
    }
};

class Visitor {
protected:
    //message tracked during visitations.
    //also used to stop traversing the AST
    //when an error occurs
    ErrorMessage message;
    void put_error(const string &msg);
public:
    bool has_errors();
    string get_errors();
    //visitors
    //default implementation: do nothing
    virtual void visit(shared_ptr<ModelAST> node);
    virtual void visit(shared_ptr<Prop> node);
    virtual void visit(shared_ptr<Model> node);
    virtual void visit(shared_ptr<ModuleBody> node);
    virtual void visit(shared_ptr<Decl> node);
    virtual void visit(shared_ptr<Action> node);
    virtual void visit(shared_ptr<Effect> node);
    virtual void visit(shared_ptr<Dist> node);
    virtual void visit(shared_ptr<Location> node);
    virtual void visit(shared_ptr<Exp> node);
    virtual void visit(shared_ptr<IConst> node);
    virtual void visit(shared_ptr<BConst> node);
    virtual void visit(shared_ptr<FConst> node);
    virtual void visit(shared_ptr<LocExp> node);
    virtual void visit(shared_ptr<OpExp> node);
};

#endif

