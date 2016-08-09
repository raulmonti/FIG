#ifndef __MODEL_AST_H__
#define __MODEL_AST_H__
#include <cassert>
#include <map>
#include <iostream>
#include <sstream>
#include "Util.h"

/** Model Abstract Syntax Tree  **/

using std::string;
using std::vector;
using std::map;

enum class Type {tint, tbool, tfloat, tclock, tunknown};
enum class ExpOp {plus, times, minus, div, mod, andd, orr, nott, eq, neq, lt, gt, le, ge};
enum class Arity  {one, two};
enum class LabelType {in, out, commited, empty};
enum class DistType {erlang, normal, uniform, exponential};

// forward declare this classes to declare them friends
// (cannot include ModelParser.hpp since its generation depends on this file)
namespace ModelParserGen {
    class ModelParser;
    class ModelScanner;
}

class ModelAST {
private:
    friend class ModelParserGen::ModelParser;
    friend class ModelParserGen::ModelScanner;
    //starts lexer on the given file
    static void scan_begin(FILE *file);
    //finish lexer
    static void scan_end();
    static void on_scanner_error(const std::string &msg);
public:
    static ModelAST *from_file(const string& filename);
    virtual ~ModelAST() {};
    virtual void accept(class Visitor& visit);
};

//@todo use shared_pointers, I tried but the .yy file became unreadable.
class Model : public ModelAST {
public:
    // (module's id -> body) map
    // public access allows the use of a VisitorPattern
    map<string, class ModuleBody *> modules;
    vector<class Decl *> globals;
    
    Model(string id, ModuleBody *mb) {
	add_module(id, mb);
    }
    
    Model(Decl *decl) {
	add_decl(decl);
    }
    
    Model(const Model &model) = delete;
    void operator=(Model const &) = delete;
    
    void add_module(string id, ModuleBody *mb) {
	modules[id] = mb;
    }
    
    void add_decl(Decl *decl) {
	globals.push_back(decl);
    }
    
    bool has_module(string id) {
	return (modules.find(id) != modules.end());
    }
    
    const map<string, ModuleBody *>& get_modules() const {
	return (modules);
    }
    
    const vector<class Decl *>& get_globals() const {
	return (globals);
    }
    
    void accept(Visitor& visit) override;
    ~Model();
};

class ModuleBody : public ModelAST {
public:
    vector<class Decl*> local_decls;
    vector<class Action*> actions;
    ModuleBody() {};
    
    ModuleBody(Decl *decl) {
	add_decl(decl);
    }
    
    ModuleBody(Action *action) {
	add_action(action);
    }
    
    ModuleBody(const ModuleBody &) = delete;
    void operator=(const ModuleBody &) = delete;
    
    void add_decl(Decl *decl) {
	local_decls.push_back(decl);
    }
    
    void add_action(Action *action) {
	actions.push_back(action);
    }
    
    void accept(Visitor& visit) override;
    
    const vector<Decl*>& get_local_decls() {
	return local_decls;
    }
    
    const vector<Action*>& get_actions() {
	return actions;
    }
    
    ~ModuleBody();
};

class Decl : public ModelAST {
public:
    Type type;
    string id;
    vector <class Exp*> inits;
    Exp *lower;
    Exp *upper;
    Exp *size;
    Decl(Type type, string id, Exp *lower, Exp *upper)
	: type {type}, id {id},
	  lower {lower}, upper {upper},size {nullptr}
	{ inits = vector<Exp*>{}; }
    Decl(Type type, string id, Exp *lower, Exp *upper, Exp *init)
	: type {type}, id {id}, lower {lower}, upper {upper},
	  size {nullptr}
	{ inits = vector<Exp*>{init}; }
    Decl(Type type, string id, Exp *init)
	: type {type}, id {id},
	  lower {nullptr}, upper {nullptr},
	  size {nullptr}
	{ inits = vector<Exp*>{init};}
    Decl(Type type, string id)
	: type {type}, id {id},
	  lower {nullptr}, upper {nullptr},
	  size {nullptr}
	{ inits = vector<Exp*>{}; }
    Decl(Type type, string id, Exp *size, Exp *lower, Exp *upper,
	 const vector<Exp*> &inits)
	: type {type}, id {id}, inits {inits},
	  lower {lower}, upper {upper},
	  size {size}
	{};
    Decl(Type type, string id, Exp *size,
	 const vector<Exp*> &inits)
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
	
    const vector<Exp*>& get_inits() {
	return (inits);
    }
	
    void accept(Visitor& visit) override;        
    ~Decl();
};
    
class Action : public ModelAST {
public:
    string id;
    LabelType type;
    Exp *guard;
    class Location *clock_loc;
    vector<class Effect *> effects;
    Action(string id, LabelType type, Exp *guard, const vector<Effect *> &effects)
	: id {id}, type {type}, guard {guard}, clock_loc {nullptr}, effects {effects} {};
    Action(string id, LabelType type, Exp *guard,
	   Location *clock_loc, const vector<Effect *> &effects)
	: id {id}, type {type}, guard {guard}, clock_loc {clock_loc}, effects {effects} {};
    Action(LabelType type, Exp *guard,
	   Location *clock_loc, const vector<Effect *> &effects)
	: id {""}, type {type}, guard {guard}, clock_loc {clock_loc}, effects {effects} {};

    Action(const Action &Decl) = delete;
    void operator=(const Action &Decl) = delete;

    void accept(Visitor& visit) override;

    bool has_clock() {
	return (clock_loc != nullptr);
    }

    const vector<Effect*>& get_effects() {
	return (effects);
    }
        
    ~Action();
};

class Effect : public ModelAST {
public:
    Location *loc;
    class Dist *dist;
    Exp *arg;
    Effect(Location *loc, Exp *arg)
	: loc {loc}, dist {nullptr}, arg {arg} {};
    Effect(Location *loc, Dist *dist)
	: loc {loc}, dist {dist}, arg {nullptr} {};
    Effect(const Effect &effect) = delete;
    void operator=(const Effect &effect) = delete;

    bool is_clock_reset() {
	return (dist != nullptr);
    }

    bool is_state_change() {
	return (arg != nullptr);
    }
        
    void accept(Visitor& visit) override;
    ~Effect();
};
    
class Dist : public ModelAST {
public:
    DistType type;
    Arity arity;
    Exp *param1;
    Exp *param2;
    Dist(DistType dist_type, Arity arity,
	 Exp *param1, Exp *param2 = nullptr) :
	type {dist_type}, arity {arity},
	param1 {param1}, param2 {param2} {};
    Dist(const Dist &) = delete;
    void operator=(const Dist &) = delete;

    void accept(Visitor& visit) override;
    ~Dist();
};
    
class Location : public ModelAST {
public:
    string id;
    Exp *index;
    Location(string id, Exp *index = nullptr) : id {id}, index {index} {};
    Location(const Location &) = delete;
    void operator=(const Location &) = delete;

    bool is_array_position() {
	return (index != nullptr);
    }

    void accept(Visitor& visit) override;
    ~Location();
};
    
class Exp : public ModelAST {
public:
    Type type;
    Exp() : type{Type::tunknown} {};
    Exp(const Exp &) = delete;
    void operator=(const Exp &) = delete;

    void accept(Visitor& visit) override;
    virtual ~Exp() {};
};

class IConst : public Exp {
public:
    int value;
    IConst(int value) : value {value} {};
    IConst(const IConst &) = delete;
    void operator=(const IConst &) = delete;

    void accept(Visitor& visit) override;
    ~IConst() {};
};

class BConst : public Exp {
public:
    bool value;
    BConst(bool value) : value {value} {};
    BConst(const BConst &) = delete;
    void operator=(const BConst &) = delete;

    void accept(Visitor& visit) override;
    ~BConst() {};
};

class FConst : public Exp {
public:
    float value;
    FConst(float value) : value {value} {};
    FConst(const FConst &) = delete;
    void operator=(const FConst &) = delete;

    void accept(Visitor& visit) override;
    ~FConst() {};
};

class LocExp : public Exp {
public:
    Location *location;
    LocExp(Location *location) : location {location} {};
        
    LocExp(const LocExp &) = delete;
    void operator=(const LocExp &) = delete;

    void accept(Visitor& visit) override;
    ~LocExp() {delete location;};
};
    
class OpExp : public Exp {
public:
    Arity arity;
    ExpOp bop;
    Exp *left;
    Exp *right;
    OpExp(Arity arity, ExpOp bop, Exp *left, Exp *right = nullptr) :
	arity {arity}, bop {bop}, left {left}, right {right} {};

    OpExp(const OpExp &) = delete;
    void operator=(const OpExp &) = delete;

    void accept(Visitor& visit) override;
    ~OpExp() {delete left; delete right;};
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
    virtual void visit(ModelAST* node);
    virtual void visit(Model* node);
    virtual void visit(ModuleBody* node);
    virtual void visit(Decl* node);
    virtual void visit(Action* node);
    virtual void visit(Effect* node);
    virtual void visit(Dist* node);
    virtual void visit(Location* node);
    virtual void visit(Exp* node);
    virtual void visit(IConst* node);
    virtual void visit(BConst* node);
    virtual void visit(FConst* node);
    virtual void visit(LocExp* node);
    virtual void visit(OpExp* node);
};

#endif

