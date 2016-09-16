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


/// @brief Types for local module variables.
enum class Type {tint, tbool, tfloat, tclock, tunknown};
/// @brief Expression operators (unary and binary)
enum class ExpOp {plus, times, minus, div, mod, andd, orr, nott,
                  eq, neq, lt, gt, le, ge};
/// @brief Arity of an operator (1 or 2)
enum class Arity  {one, two};

/// @brief Type of labels allowed in transitions.
/// @todo commited actions not yet supported
enum class LabelType {in, out, commited, empty};
/// @brief Supported distributions
enum class DistType {erlang, normal, lognormal, uniform, exponential,
                     weibull, rayleigh, gamma};
/// @brief Supported properties
enum class PropType {transient, rate};

// forward declare this classes to declare them friends
// (cannot include ModelParser.hpp since its generation depends on this file)
namespace ModelParserGen {
class ModelParser;
class ModelScanner;
class location;
}

using ModelParserGen::location;

/**
 * @brief The ModelAST class. This class represents an abstract syntax tree of
 * the model.
 * @details Every node of inherits this class. Instances of this class are built
 * in the file ModelParser.cpp
 * (generated by bison and the template ModelParserGen.yy)
 */
class ModelAST : public std::enable_shared_from_this<ModelAST> {
public:
    virtual ~ModelAST() {}
private:
    friend class ModelParserGen::ModelParser;
    friend class ModelParserGen::ModelScanner;

    /// Starts lexer on the given file
    static void scan_begin(FILE *file);

    /// Ends lexer (defined in ModelParserGen.yy)
    static void scan_end();

    /// Prints error message if a lexer error occurs.
    static void on_scanner_error(const std::string &msg);

    /// Location from which this ast was created.
    /// save it only for improve error messages.
    shared_ptr<location> token_loc = nullptr;
public:

	/// Build an AST from two files corresponding to the model and
	/// the properties (e.g. tandem-queue.sa, tandem-queue.pp)
    static shared_ptr<ModelAST> from_files(const char *model_file,
										   const char *prop_file);

    /// Acceptor
    /// @see https://en.wikipedia.org/wiki/Visitor_pattern
    virtual void accept(class Visitor& visit);

    /// Save location of the first token that
    /// was used to generate this AST.
    /// @todo not yet used
    void set_location(shared_ptr<location> loc) {
        this->token_loc = loc;
    }

    /// Returns the location of the first token
    /// that generated this AST.
    shared_ptr<location> get_location() {
        return (this->token_loc);
    }
};

/**
 * @brief Properties of the Model.
 */
class Prop : public ModelAST {
public:
    /// Type of the property (Transient, Rate)
    PropType type;
    /// Left expression. In Transient properties this
    /// corresponds to the expression at the left of
    /// the "until" operator "U".
    shared_ptr<class Exp> left;
    /// Right expression
    shared_ptr<class Exp> right;
    
    /// Constructs a Rate property.
    Prop(shared_ptr<Exp> rate)
        : type {PropType::rate}, left {rate}, right {nullptr} {}

    /// Constructs a Transient property.
    Prop(shared_ptr<Exp> left, shared_ptr<Exp> right)
        : type {PropType::transient}, left {left}, right {right} {}

    /// Acceptor.
    void accept(Visitor& visit) override;
};

/**
 * @brief The root of the AST. Contains declaration of
 *        global constants, modules and properties.
 */
class Model : public ModelAST {
public:
    // public access allows an easier implementation of VisitorPattern

    /// Modules of the model.
    shared_vector<class ModuleBody> modules;
    /// List of modules names.
    vector<string> modules_ids;
	/// Global declarations (currently just constants)
    shared_vector<class Decl> globals;
    /// Properties of the model.
    shared_vector<Prop> props;
    
    /// Construct a model from a single module
    Model(string id, shared_ptr<ModuleBody> mb) {
        add_module(id, mb);
    }
    
    /// Construct a model from a single declaration.
    Model(shared_ptr<Decl> decl) {
        add_decl(decl);
    }
    
    /// Copy and Assignment constructor are disabled.
    Model(const Model &model) = delete;
    void operator=(Model const &) = delete;
    
    /// Adds a module to the model.
    void add_module(string id, shared_ptr<ModuleBody> mb) {
        modules.push_back(mb);
        modules_ids.push_back(id);
    }
    
    /// Adds a declaration to the model.
    void add_decl(shared_ptr<Decl> decl) {
        globals.push_back(decl);
    }

    /// Adds a vector of properties to the model.
    void add_props(const shared_vector<Prop> &properties) {
        props.insert(props.end(), properties.begin(), properties.end());
    }
    
    /// Is there a model with the given name?
    bool has_module(const string& id) {
        return (std::find(modules_ids.begin(), modules_ids.end(), id)
                != modules_ids.end());
    }
    
    /// Returns the modules of this model
    const shared_vector<ModuleBody> &get_modules() const {
        return (modules);
    }

    /// Returns the names of the modules of this model
    const vector<string> &get_modules_ids() const {
        return (modules_ids);
    }
    
    /// Returns the global declarations of this model
    const vector<shared_ptr<class Decl>>& get_globals() const {
        return (globals);
    }

    /// Get properties of this model
    const vector<shared_ptr<Prop>>& get_props() const {
        return (props);
    }

    /// Are there properties in the model?
    bool has_props() {
        return (props.size() > 0);
    }
    
    /// Acceptor.
    void accept(Visitor& visit) override;
};

/**
 * @brief A Module AST. Contains local declarations,
 *        and transitions.
 */
class ModuleBody : public ModelAST {
public:
    /// Local declarations of the model.
    shared_vector<Decl> local_decls;

    /// Transitions of the model.
    shared_vector<class Action> actions;
    
    /// Default constructor
    ModuleBody() {}
    
    /// Create a module from a single declaration
    ModuleBody(shared_ptr<Decl> decl) {
        add_decl(decl);
    }
    
    /// Create a module from a single transition (action)
    ModuleBody(shared_ptr<Action> action) {
        add_action(action);
    }
    
    ModuleBody(const ModuleBody &) = delete;
    void operator=(const ModuleBody &) = delete;
    
    /// Add a local declaration to the module
    void add_decl(shared_ptr<Decl> decl) {
        local_decls.push_back(decl);
    }
    
    /// Add a transition to the module
    void add_action(shared_ptr<Action> action) {
        actions.push_back(action);
    }
    
    /// Acceptor
    void accept(Visitor& visit) override;
    
    /// Returns local declaration of the model.
    const shared_vector<Decl>& get_local_decls() {
        return local_decls;
    }
    
    /// Returns the transitions of the module.
    const shared_vector<Action>& get_actions() {
        return actions;
    }
};

/** @brief A declaration AST. Contains the
 *         type, the name, the range of the declaration
 */
class Decl : public ModelAST {
public:
	/// The type of the declaration: tint, tbool, tfloat, tclock, tunknown.
	Type type;

    /// The name of the declaration (identifier)
    string id;

    /// The list of initialization of this declaration.
    /// @note This is a list for a future implementation of arrays.
    /// @note This is an empty list when the declaration is a clock.
    shared_vector<class Exp> inits;

    /// Lower bound of the state variable.
    /// @note equal to nullptr when this is a clock declaration,
    /// or a boolean state variable.
    shared_ptr<Exp> lower;

    /// Upper bound of the state variable.
    /// @note nullptr when this is a clock or boolean declaration
    shared_ptr<Exp> upper;

    /// Size of the array
    /// @note Not yet used.
    shared_ptr<Exp> size;
    
    /// Declaration with type, id, range, and no initialization.
    /// (e.g  x :[4...8])
    /// @note initialization is taken to be the lower bound of the range.
    Decl(Type type, string id, shared_ptr<Exp> lower, shared_ptr<Exp> upper)
        : type {type}, id {id},
          lower {lower}, upper {upper},size {nullptr}
    {
        //choose lower limit as initialization
        //note: this turns the AST into an ASGraph,
        //safe only by using shared_ptr or a well-designed deleter.
        inits = vector<shared_ptr<Exp>>{lower};
    }
    
    /// Declaration with type, id, range and initialization
    /// (e.g x : [4 .. 8] init 3)
    Decl(Type type, string id, shared_ptr<Exp> lower,
         shared_ptr<Exp> upper, shared_ptr<Exp> init)
        : type {type}, id {id}, lower {lower}, upper {upper},
          size {nullptr}
    { inits = vector<shared_ptr<Exp>>{init}; }

    /// Declaration with type, id, initialization (no range)
    /// (e.g x : bool init false)
    /// @note lower and upper members are equal to nullptr
    Decl(Type type, string id, shared_ptr<Exp> init)
        : type {type}, id {id},
          lower {nullptr}, upper {nullptr},
          size {nullptr}
    { inits = vector<shared_ptr<Exp>>{init};}

    /// Declaration of a clock
    /// (e.g c : clock)
    Decl(Type type, string id)
        : type {type}, id {id},
          lower {nullptr}, upper {nullptr},
          size {nullptr}
    { inits = vector<shared_ptr<Exp>>{}; }

    /// Declaration of array with id, size of array,
    ///  range of values, initalizations.
    Decl(Type type, string id, shared_ptr<Exp> size,
         shared_ptr<Exp> lower, shared_ptr<Exp> upper,
         const shared_vector<Exp> &inits)
        : type {type}, id {id}, inits {inits},
          lower {lower}, upper {upper},
          size {size} {}

    /// Declaration of array with id, size of the array,
    /// no range for the values
    /// and the corresponding initialization
    Decl(Type type, string id, shared_ptr<Exp> size,
         const shared_vector<Exp> &inits)
        : type {type}, id {id}, inits {inits},
          lower {nullptr}, upper {nullptr},
          size {size} {}

    /// Copy constructor deleted
    Decl(const Decl &Decl) = delete;
    void operator=(const Decl &Decl) = delete;

    /// Check if the variable was declared with a range for the values.
    bool has_range() {
        return (lower != nullptr);
    }

    /// Is this declaration an array?
    bool is_array() {
        return (size != nullptr);
    }

    /// Has this variable an initialization ?
    /// @note: should be always true by construction since,
    /// when the initialization is not provided, the lower
    /// bound is taken as default initialization.
    bool has_single_init() {
        return (inits.size() == 1);
    }

    /// Has this array an initialization ?
    bool has_array_init() {
        return (inits.size() > 1);
    }

    /// Vector of initizalitions
    const shared_vector<Exp>& get_inits() {
        return (inits);
    }

    /// Acceptor
    void accept(Visitor& visit) override;
};

/** @brief Transition of the module.
 */
class Action : public ModelAST {
public:
    /// Name of the label.
    string id;
    /// Type of the transition (input, output, commited)
    LabelType type;
    /// Precondition of the transition
    /// @note: when the parser finds no precondition, "true" is the default.
    shared_ptr<Exp> guard;
    /// Location of the triggering clock
    shared_ptr<class Location> clock_loc;
    /// Vector of effects (assignments or clock reset)
    shared_vector<class Effect> effects;

    /// Construct an input transition.
    /// e.g  [id ? ] guard -> (effect1 & effect2 & .... )
    Action(string id, LabelType type, shared_ptr<Exp> guard,
           const shared_vector<Effect> &effects)
        : id {id}, type {type}, guard {guard},
          clock_loc {nullptr}, effects {effects} {}

    /// Construct an output transition.
    /// e.g [id ! ] guard @ clock -> (effect1 & effect2 & .... )
    Action(string id, LabelType type, shared_ptr<Exp> guard,
           shared_ptr<Location> clock_loc, const shared_vector<Effect> &effects)
        : id {id}, type {type}, guard {guard},
          clock_loc {clock_loc}, effects {effects} {}

    /// Construct an input transition without label
    /// e.g [] guard @ clock -> (effect1 & effect2 & .... )
    Action(LabelType type, shared_ptr<Exp> guard,
           shared_ptr<Location> clock_loc, const shared_vector<Effect> &effects)
        : id {""}, type {type}, guard {guard},
          clock_loc {clock_loc}, effects {effects} {}
    
    Action(const Action &Decl) = delete;
    void operator=(const Action &Decl) = delete;

    /// Acceptor
    void accept(Visitor& visit) override;

    /// Has this transition a triggering clock?
    /// @note The language grammar forces that every output transition
    /// must have a triggering clock, and every input transition must not.
    bool has_clock() {
        return (clock_loc != nullptr);
    }

    /// Transition effects (assingments or clock resets)
    const shared_vector<Effect>& get_effects() {
        return (effects);
    }
};


/** @brief Effects of a transition (assginments or clock resets)
 */
class Effect : public ModelAST {
public:
    /// The location in which the changes are made (name of the
    /// state variable or the clock).
    shared_ptr<Location> loc;

    /// The probability distribution of the clock
    shared_ptr<class Dist> dist;

    /// The expression that should become the new value of the state.
    shared_ptr<Exp> arg;

    /// Create an assignment (state variable change)
    Effect(shared_ptr<Location> loc, shared_ptr<Exp> arg)
        : loc {loc}, dist {nullptr}, arg {arg} {}

    /// Create a clock reset effect
    Effect(shared_ptr<Location> loc, shared_ptr<Dist> dist)
        : loc {loc}, dist {dist}, arg {nullptr} {}
    
    Effect(const Effect &effect) = delete;
    void operator=(const Effect &effect) = delete;

    /// Is this effect a clock reset ?
    bool is_clock_reset() const {
        return (dist != nullptr);
    }

    /// Is this effect an assignment ?
    bool is_state_change() const {
        return (arg != nullptr);
    }

    /// Acceptor
    void accept(Visitor& visit) override;
};

/** @brief Probability distributions */
class Dist : public ModelAST {
public:
    /// Distribution type
    DistType type;

    /// The arity (number of arguments)
    Arity arity;

    /// The first parameter
    shared_ptr<Exp> param1;

    /// The second parameter (nullptr if arity is Arity::one)
    shared_ptr<Exp> param2;

    /// Create a distribution from type, arity and parameters.
    Dist(DistType dist_type, Arity arity,
         shared_ptr<Exp> param1, shared_ptr<Exp> param2 = nullptr) :
        type {dist_type}, arity {arity},
        param1 {param1}, param2 {param2} {}
    
    Dist(const Dist &) = delete;
    void operator=(const Dist &) = delete;

    /// Acceptor
    void accept(Visitor& visit) override;
};

/**
 * @brief A location in the state: an identifier (e.g "x")
 * or an indexed array (e.g "x [4]")
 */
class Location : public ModelAST {
public:
    /// The identifier
    string id;

    /// Expression used to compute the index
    /// @note: must be nullptr if the location is not an array
    shared_ptr<Exp> index;

    /// Create a location from an identifier and an optional index
    Location(string id, shared_ptr<Exp> index = nullptr)
        : id {id}, index {index} {}
    
    Location(const Location &) = delete;
    void operator=(const Location &) = delete;

    /// Is this location an array position?
    bool is_array_position() {
        return (index != nullptr);
    }

    /// Acceptor
    void accept(Visitor& visit) override;
};

/**
 * @brief Expressions base class.
 */
class Exp : public ModelAST {
public:
    /// Type of the expression
    /// @note This member is setted by ModelTC (typechecking)
    /// or by the parser when the type is inferred by the syntax,
    /// by default is equal to Type::tunknown
    Type type;

    Exp() : type{Type::tunknown} {}

    Exp(const Exp &) = delete;
    void operator=(const Exp &) = delete;

    /// Acceptor
    void accept(Visitor& visit) override;
};

/**
 * @brief A constant integer expression
 */
class IConst : public Exp {
public:
    /// The value of the constant
    int value;

    /// Construct the constant
    IConst(int value) : value {value} {
        type = Type::tint;
    }

    IConst(const IConst &) = delete;
    void operator=(const IConst &) = delete;

    /// Acceptor
    void accept(Visitor& visit) override;
};

/**
 * @brief A boolean constant
 */
class BConst : public Exp {
public:
    /// The value of the boolean constant
    bool value;

    /// Create the boolean constant from its value
    BConst(bool value) : value {value} {
        type = Type::tbool;
    }

    BConst(const BConst &) = delete;
    void operator=(const BConst &) = delete;

    /// Acceptor
    void accept(Visitor& visit) override;
};

/**
 * @brief A float constant
 */
class FConst : public Exp {
public:
    /// The value of the float constant
    float value;

    /// Create the float constant from its value
    FConst(float value) : value {value} {
        type = Type::tfloat;
    }

    FConst(const FConst &) = delete;
    void operator=(const FConst &) = delete;

    /// Acceptor
    void accept(Visitor& visit) override;
};

/**
 * @brief A location expression.
 */
class LocExp : public Exp {
public:
	/// The location that should be read
    /// to obtain the value of this expression
    shared_ptr<Location> location;

    /// Create a location expression
    LocExp(shared_ptr<Location> location) : location {location} {}

    LocExp(const LocExp &) = delete;
    void operator=(const LocExp &) = delete;

    /// Acceptor
    void accept(Visitor& visit) override;
};

/**
 * @brief A (unary, binary) operator expression.
 */
class OpExp : public Exp {
public:
    /// Arity (one or two)
    Arity arity;

    /// Operator
    ExpOp bop;

    /// Left argument
    shared_ptr<Exp> left;

    /// Right argument (if any, or nullptr)
    shared_ptr<Exp> right;

    /// Create an OpExp from arity, operator and arguments.
    OpExp(Arity arity, ExpOp bop, shared_ptr<Exp> left,
          shared_ptr<Exp> right = nullptr) :
        arity {arity}, bop {bop}, left {left}, right {right} {}

    OpExp(const OpExp &) = delete;
    void operator=(const OpExp &) = delete;

    /// Acceptor
    void accept(Visitor& visit) override;

    /// Create a expression that represents the negation of the given argument
    static shared_ptr<Exp> make_nott(shared_ptr<Exp> exp) {
        return make_shared<OpExp>(Arity::one, ExpOp::nott, exp);
    }

    /// Create a expression that represents the
    /// conjuction of the given arguments
    static shared_ptr<Exp> make_andd(shared_ptr<Exp> exp1,
                                     shared_ptr<Exp> exp2) {
        return make_shared<OpExp>(Arity::two, ExpOp::andd, exp1, exp2);
    }
};


/**
 * @brief Visitor base class. Every other class that manipulates
 * the AST is a subclass of Visitor. This is used to implement
 * the VisitorPattern idiom. The default implementation is to do
 * nothing on each node visitation.
 */
class Visitor {
protected:
    /// An error message to show to the user, also used to stop AST traversing
    /// when an error is found.
    shared_ptr<class ErrorMessage> message;

    /// Signal an error, store the message to show to the
    /// user when the AST traversing is finished.
    void put_error(const string &msg);
    void put_warning(const string &msg);

public:
    /// Default constructor
    Visitor();

    /// Has this visitor an error? Is the AST traversing incomplete?
    bool has_errors();
    bool has_warnings();

    /// Returns a string with the error message.
    /// @note call only if has_errors() or has_warnings() is true.
    string get_messages();

	/// Visitor functions, one for each node
	/// @note These aren't pure-virtual to avoid forcing implementation.
	///       Instead they're implemented as default-NOP virtual functions
	virtual void visit(shared_ptr<ModelAST>)   { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<Prop>)       { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<Model>)      { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<ModuleBody>) { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<Decl>)       { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<Action>)     { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<Effect>)     { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<Dist>)       { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<Location>)   { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<Exp>)        { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<IConst>)     { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<BConst>)     { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<FConst>)     { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<LocExp>)     { /* NOP */ }
	/// @copydoc visit(shared_ptr<ModelAST>)
	virtual void visit(shared_ptr<OpExp>)      { /* NOP */ }
};

#endif

