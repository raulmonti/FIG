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
/// @brief Type of labels allowed in transitions.
enum class LabelType {in, out, out_committed, in_committed, tau};
/// @brief Supported distributions
enum class DistType {erlang, normal, lognormal, uniform, exponential,
                     weibull, rayleigh, gamma};
/// @brief Supported properties
enum class PropType {transient, rate};

/// @brief Declaration qualifiers
enum class DeclQualifier {constant};

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
private:
    /// Type of the property (Transient, Rate)
    PropType type;
protected:
    Prop(PropType type) : type {type} {}

public:
    PropType get_type() {
        return (type);
    }
    /// Acceptor.
    virtual void accept(Visitor& visit) override;

    shared_ptr<class TransientProp> to_transient() {
        assert(type == PropType::transient);
        return std::static_pointer_cast<TransientProp>(shared_from_this());
    }

    shared_ptr<class RateProp> to_rate() {
        assert(type == PropType::rate);
        return std::static_pointer_cast<RateProp>(shared_from_this());
    }
};

class TransientProp : public Prop {
private:
    /// Left expression.
    shared_ptr<class Exp> left;
    /// Right expression
    shared_ptr<class Exp> right;
public:
    TransientProp(shared_ptr<Exp> left, shared_ptr<Exp> right) :
        Prop(PropType::transient), left {left}, right {right} {}

    shared_ptr<Exp> get_left() {
        return (left);
    }

    shared_ptr<Exp> get_right() {
        return (right);
    }

    /// Acceptor.
    virtual void accept(Visitor& visit) override;
};

class RateProp : public Prop {
private:
    /// Expression
    shared_ptr<Exp> exp;
public:
    RateProp(shared_ptr<Exp> exp) : Prop(PropType::rate), exp {exp} {}

    shared_ptr<Exp> get_expression() {
        return (exp);
    }

    /// Acceptor.
    virtual void accept(Visitor& visit) override;
};


/**
 * @brief The root of the AST. Contains declaration of
 *        global constants, modules and properties.
 */
class Model : public ModelAST {
private:
    /// Modules of the model.
    shared_vector<class ModuleAST> modules;
    /// Global declarations
    shared_vector<class Decl> globals;
    /// Properties of the model.
    shared_vector<Prop> props;
public:
    /// Construct a model from a single module
    Model(shared_ptr<ModuleAST> mb) {
        add_module(mb);
    }
    
    /// Construct a model from a single declaration.
    Model(shared_ptr<Decl> decl) {
        add_decl(decl);
    }
    
    /// Copy and Assignment constructor are disabled.
    Model(const Model &model) = delete;
    void operator=(Model const &) = delete;
    
    /// Adds a module to the model.
    void add_module(shared_ptr<ModuleAST> mb) {
        modules.push_back(mb);
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
    bool has_module(const string& id);

    /// Returns the modules of this model
    const shared_vector<ModuleAST> &get_modules() const {
        return (modules);
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
    virtual void accept(Visitor& visit) override;
};

/**
 * @brief A Module AST. Contains local declarations,
 *        and transitions.
 */
class ModuleAST : public ModelAST {
private:
    /// Module Name
    string id;

    /// Local declarations of the model.
    shared_vector<Decl> local_decls;

    /// Transitions of the model.
    shared_vector<class TransitionAST> transitions;
public:
    /// Default constructor
    ModuleAST() {}
    
    /// Create a module from a single declaration
    ModuleAST(shared_ptr<Decl> decl) {
        add_decl(decl);
    }
    
    /// Create a module from a single transition
    ModuleAST(shared_ptr<TransitionAST> transition) {
        add_transition(transition);
    }
    
    ModuleAST(const ModuleAST &) = delete;
    void operator=(const ModuleAST &) = delete;
    
    /// Add a local declaration to the module
    void add_decl(shared_ptr<Decl> decl) {
        local_decls.push_back(decl);
    }
    
    /// Add a transition to the module
    void add_transition(shared_ptr<TransitionAST> transition) {
        transitions.push_back(transition);
    }
    
    /// Acceptor
    virtual void accept(Visitor& visit) override;
    
    /// Returns local declaration of the model.
    const shared_vector<Decl>& get_local_decls() {
        return local_decls;
    }
    
    /// Returns the transitions of the module.
    const shared_vector<TransitionAST>& get_transitions() {
        return transitions;
    }

    string get_name() {
        return (id);
    }

    void set_name(const string &name) {
        this->id = name;
    }
};

class Initialized {
private:
    shared_ptr<Exp> init;
public:
    Initialized(shared_ptr<Exp> init) :
       init {init} {}
    shared_ptr<Exp> get_init() {
        return (init);
    }
};

class MultipleInitialized {
private:
    shared_vector<Exp> inits;
public:
    MultipleInitialized(const shared_vector<Exp>& inits)
        : inits {inits} {}
    const shared_vector<Exp>& get_inits() {
        return (inits);
    }
};

class Ranged {
private:
    shared_ptr<Exp> lower;
    shared_ptr<Exp> upper;
public:
    Ranged(shared_ptr<Exp> lower, shared_ptr<Exp> upper):
        lower {lower}, upper {upper} {}
    shared_ptr<Exp> get_lower_bound() {
        return (lower);
    }
    shared_ptr<Exp> get_upper_bound() {
        return (upper);
    }
};

/** @brief A declaration AST */
class Decl : public ModelAST {
private:
    Type type;
    string id;
    std::vector<DeclQualifier> qualifiers;

protected: //Protected constructor
    Decl(Type type, string id):
        type {type}, id {id} {}
public:
    virtual void accept(Visitor& visit) override;

    void add_qualifier(DeclQualifier qualifier) {
        qualifiers.push_back(qualifier);
    }

    bool is_constant() {
       const auto &it = std::find(qualifiers.begin(),
               qualifiers.end(), DeclQualifier::constant);
       return (it != qualifiers.end());
    }

    void mark_as_constant() {
        add_qualifier(DeclQualifier::constant);
    }

    Type get_type() {
        return (type);
    }

    string get_id() {
        return (id);
    }

    virtual bool has_range() {
        return (false);
    }

    virtual bool has_init() {
        return (false);
    }

    shared_ptr<class RangedDecl> to_ranged() {
        assert(has_range());
        return std::static_pointer_cast<RangedDecl>(shared_from_this());
    }

    shared_ptr<class InitializedDecl> to_initialized() {
        assert(has_init());
        return std::static_pointer_cast<InitializedDecl>(shared_from_this());
    }
};

class InitializedDecl : public Decl, public Initialized {
public:
    InitializedDecl(Type type, string id, shared_ptr<Exp> init)
        : Decl(type, id), Initialized(init) {}
    virtual void accept(Visitor& visit) override;
    virtual bool has_init() {
        return (true);
    }
};

class RangedDecl : public Decl, public Initialized, public Ranged {
public:
    RangedDecl(string id, shared_ptr<Exp> lower, shared_ptr<Exp> upper,
               shared_ptr<Exp> init) :
        Decl(Type::tint, id), Initialized(init), Ranged(lower, upper) {}

    RangedDecl(string id, shared_ptr<Exp> lower, shared_ptr<Exp> upper) :
        Decl(Type::tint, id), Initialized(lower), Ranged(lower, upper) {}

    virtual void accept(Visitor& visit) override;

    bool has_range() override {
        return (true);
    }

    bool has_init() override {
        return (true);
    }
};

class ClockDecl : public Decl {
    //here we could expect an optional distribution type.
public:
    ClockDecl(string id) : Decl(Type::tclock, id) {}
    virtual void accept(Visitor& visit) override;
};

class ArrayDecl : public Decl {
private:
    shared_ptr<Exp> size;
protected:
    ArrayDecl(Type type, string id, shared_ptr<Exp> size)
        : Decl(type, id), size {size} {}
public:
    shared_ptr<Exp> get_size() {
        return (size);
    }
    virtual void accept(Visitor& visit) override;
};

class InitializedArray : public ArrayDecl, public Initialized {
public:
    InitializedArray(Type type, string id, shared_ptr<Exp> size,
                shared_ptr<Exp> init) :
        ArrayDecl(type, id, size), Initialized {init} {}
    virtual void accept(Visitor& visit) override;
};

class MultipleInitializedArray : public ArrayDecl, public MultipleInitialized {
public:
    MultipleInitializedArray(Type type, string id, shared_ptr<Exp> size,
                                 const shared_vector<Exp>& inits) :
        ArrayDecl(type, id, size), MultipleInitialized {inits} {}
    virtual void accept(Visitor& visit) override;
};

class RangedInitializedArray : public ArrayDecl,
        public Initialized, public Ranged {
public:
    RangedInitializedArray(string id, shared_ptr<Exp> size,
                              shared_ptr<Exp> lower, shared_ptr<Exp> upper,
                              shared_ptr<Exp> init) :
        ArrayDecl(Type::tint, id, size),
        Initialized(init), Ranged(lower, upper) {}
    virtual void accept(Visitor& visit) override;
};

class RangedMultipleInitializedArray :
        public ArrayDecl, public MultipleInitialized, public Ranged {
public:
    RangedMultipleInitializedArray(string id, shared_ptr<Exp> size,
                                shared_ptr<Exp> lower, shared_ptr<Exp> upper,
                                const shared_vector<Exp>& inits) :
        ArrayDecl(Type::tint, id, size),
        MultipleInitialized(inits), Ranged(lower, upper) {}
    virtual void accept(Visitor& visit) override;
};

/** @brief Transition of the module.
 */
class TransitionAST : public ModelAST {
protected:
    /// Name of the label.
    string id;
    /// Type of the transition (input, output, commited)
    LabelType type;
    /// Precondition of the transition
    /// @note: when the parser finds no precondition, "true" is the default.
    shared_ptr<Exp>  precondition;
    /// Vector of effects (assignments or clock reset)
    shared_vector<class Assignment> assignments;
    shared_vector<class ClockReset> clock_resets;

protected: //Protected Constructor
    TransitionAST(string label_id, LabelType type, shared_ptr<Exp> pre,
           const shared_vector<class Effect> &effects);

    TransitionAST(const TransitionAST &Decl) = delete;
    void operator=(const TransitionAST &Decl) = delete;

public:
    /// Acceptor
    virtual void accept(Visitor& visit) override;

    string get_label() {
        return (id);
    }

    const shared_vector<Assignment>& get_assignments() {
        return (assignments);
    }

    const shared_vector<ClockReset>& get_clock_resets() {
        return (clock_resets);
    }

    shared_ptr<Exp> get_precondition() {
        return (precondition);
    }

    virtual bool has_triggering_clock() {
        return (false);
    }

    shared_ptr<class OutputTransition> to_output() {
        assert(has_triggering_clock());
        return std::static_pointer_cast<OutputTransition>(shared_from_this());
    }

    shared_ptr<class InputTransition> to_input() {
        assert(type == LabelType::in);
        return std::static_pointer_cast<InputTransition>(shared_from_this());
    }

    LabelType get_label_type() {
        return (type);
    }
};

class OutputTransition : public TransitionAST {
private:
    /// Location of Triggering clock
    shared_ptr<class Location> clock_loc;
public:
    OutputTransition(string label_id, shared_ptr<Exp> pre,
                     const shared_vector<Effect> &effects,
                     shared_ptr<Location> clock_loc) :
        TransitionAST(label_id, LabelType::out, pre, effects),
        clock_loc {clock_loc} {}

    shared_ptr<Location> get_triggering_clock() {
        return (clock_loc);
    }

    virtual bool has_triggering_clock() override {
        return (true);
    }

    /// Acceptor
    virtual void accept(Visitor& visit) override;
};

class TauTransition : public OutputTransition {
public:
    TauTransition(shared_ptr<Exp> pre, const shared_vector<Effect>& effects,
                  shared_ptr<Location> clock_loc) :
        OutputTransition("", pre, effects, clock_loc) {
        type = LabelType::tau;
    }

    /// Acceptor
    virtual void accept(Visitor& visit) override;
};

class InputTransition : public TransitionAST {
public:
    InputTransition(string label_id, shared_ptr<Exp> pre,
                    const shared_vector<Effect> &effects) :
        TransitionAST(label_id, LabelType::in, pre, effects) {}
    /// Acceptor
    virtual void accept(Visitor& visit) override;
};

class InputCommittedTransition : public TransitionAST {
public:
    InputCommittedTransition(string label_id, shared_ptr<Exp> pre,
                             const shared_vector<Effect> &effects) :
    TransitionAST(label_id, LabelType::in_committed, pre, effects) {}
    /// Acceptor
    virtual void accept(Visitor& visit) override;
};

class OutputCommittedTransition : public TransitionAST {
public:
    OutputCommittedTransition(string label_id, shared_ptr<Exp> pre,
                              const shared_vector<Effect> &effects) :
        TransitionAST(label_id, LabelType::out_committed, pre, effects) {}
    /// Acceptor
    virtual void accept(Visitor& visit) override;
};

/** @brief Effects of a transition (assginments or clock resets)
 */
class Effect : public ModelAST {
protected:
    /// The location in which the changes are made (name of the
    /// state variable or the clock).
    shared_ptr<Location> loc;
protected: //Protected Constructor
    Effect(shared_ptr<Location> loc) : loc {loc} {}

    Effect(const Effect &effect) = delete;
    void operator=(const Effect &effect) = delete;
public:
    virtual void accept(Visitor& visit) override;

    virtual bool is_clock_reset() {
        return (false);
    }

    virtual bool is_assignment() {
        return (false);
    }

    shared_ptr<Location> get_effect_location() {
        return (loc);
    }
};

class Assignment : public Effect {
private:
    /// The expression that should become the new value of the state.
    shared_ptr<Exp> rhs;
public:
    Assignment(shared_ptr<Location> state_loc, shared_ptr<Exp> rhs)
        : Effect(state_loc), rhs {rhs} {}
    shared_ptr<Exp> get_rhs() {
        return (rhs);
    }
    bool is_assignment() override {
        return (true);
    }
    virtual void accept(Visitor& visit) override;
};

class ClockReset : public Effect {
private:
    /// The probability distribution of the clock
    shared_ptr<class Dist> dist;
public:
    ClockReset(shared_ptr<Location> clock_loc, shared_ptr<Dist> dist):
        Effect (clock_loc),  dist {dist} {}
    shared_ptr<Dist> get_dist() {
        return (dist);
    }
    bool is_clock_reset() override {
        return (true);
    }
    virtual void accept(Visitor& visit) override;
};

/** @brief Probability distributions */
class Dist : public ModelAST {
private:
    /// Distribution type
    DistType type;

protected: //protected constructor
    Dist(DistType type) : type {type} {}

    Dist(const Dist &) = delete;
    void operator=(const Dist &) = delete;

public:
    /// Acceptor
    virtual void accept(Visitor& visit) override;

    DistType get_type() {
        return (type);
    }

    virtual bool has_single_parameter() {
        return (false);
    }

    virtual bool has_multiple_parameters() {
        return (false);
    }

    shared_ptr<class SingleParameterDist> to_single_parameter() {
        assert(has_single_parameter());
        return std::static_pointer_cast<SingleParameterDist>(shared_from_this());
    }

    shared_ptr<class MultipleParameterDist> to_multiple_parameter() {
        assert(has_multiple_parameters());
        return std::static_pointer_cast<MultipleParameterDist>(shared_from_this());
    }
};

class SingleParameterDist : public Dist {
private:
    shared_ptr<Exp> param;
public:
    SingleParameterDist(DistType type, shared_ptr<Exp> param):
        Dist(type), param {param} {}

    shared_ptr<Exp> get_parameter() {
        return (param);
    }

    virtual bool has_single_parameter() override {
        return (true);
    }

    virtual void accept(Visitor& visit) override;
};

class MultipleParameterDist : public Dist {
private:
    shared_ptr<Exp> param1;
    shared_ptr<Exp> param2;
public:
    MultipleParameterDist(DistType type,
                        shared_ptr<Exp> param1, shared_ptr<Exp> param2) :
        Dist(type), param1 {param1}, param2 {param2} {}

    shared_ptr<Exp> get_first_parameter() {
        return (param1);
    }

    shared_ptr<Exp> get_second_parameter() {
        return (param2);
    }

    virtual bool has_multiple_parameters() override {
        return (true);
    }

    virtual void accept(Visitor& visit) override;
};

/**
 * @brief A location in the state: an identifier (e.g "x")
 * or an indexed array (e.g "x [4]")
 */
class Location : public ModelAST {
private:
    /// The identifier
    string id;
public:
    Location(const string& id) : id {id} {}
    Location(const Location &) = delete;
    void operator=(const Location &) = delete;

public:
    /// Acceptor
    virtual void accept(Visitor& visit) override;
    string get_identifier() {
        return (id);
    }
};

class ArrayPosition : public Location {
private:
    /// Expression used to compute the index
    shared_ptr<Exp> index;
public:
    ArrayPosition(const string &id, shared_ptr<Exp> index)
        : Location(id), index {index} {}
    /// Acceptor
    virtual void accept(Visitor& visit) override;
    shared_ptr<Exp> get_index() {
        return (index);
    }
};

/**
 * @brief Expressions base class.
 */
class Exp : public ModelAST {
protected:
    /// Type of the expression
    /// @note This member is setted by ModelTC (typechecking)
    /// or by the parser when the type is inferred by the syntax,
    /// by default is equal to Type::tunknown
    Type type;
    Exp() : type{Type::tunknown} {}
public:
    Exp(const Exp &) = delete;
    void operator=(const Exp &) = delete;

    /// Acceptor
    virtual void accept(Visitor& visit) override;

    Type get_type() {
        return (type);
    }

    void set_type(Type type) {
        this->type = type;
    }
};

/**
 * @brief A constant integer expression
 */
class IConst : public Exp {
private:
    int value;
public:
    /// Construct the constant
    IConst(int value) : value {value} {
        type = Type::tint;
    }

    IConst(const IConst &) = delete;
    void operator=(const IConst &) = delete;

    /// Acceptor
    virtual void accept(Visitor& visit) override;

    int get_value() {
        return (value);
    }
};

/**
 * @brief A boolean constant
 */
class BConst : public Exp {
private:
    bool value;
public:
    /// Create the boolean constant from its value
    BConst(bool value) : value {value} {
        type = Type::tbool;
    }

    BConst(const BConst &) = delete;
    void operator=(const BConst &) = delete;

    /// Acceptor
    virtual void accept(Visitor& visit) override;

    bool get_value() {
        return (value);
    }
};

/**
 * @brief A float constant
 */
class FConst : public Exp {
private:
    float value;
public:
    /// Create the float constant from its value
    FConst(float value) : value {value} {
        type = Type::tfloat;
    }

    FConst(const FConst &) = delete;
    void operator=(const FConst &) = delete;

    /// Acceptor
    virtual void accept(Visitor& visit) override;

    float get_value() {
        return (value);
    }
};

/**
 * @brief A location expression.
 */
class LocExp : public Exp {
private:
    /// The location that should be read
    /// to obtain the value of this expression
    shared_ptr<Location> location;
public:
    /// Create a location expression
    LocExp(shared_ptr<Location> location) : location {location} {}

    LocExp(const LocExp &) = delete;
    void operator=(const LocExp &) = delete;

    /// Acceptor
    virtual void accept(Visitor& visit) override;

    shared_ptr<Location> get_exp_location() {
        return (location);
    }
};

/**
 * @brief A operator expression.
 */
class OpExp : public Exp {
private:
    /// Operator
    ExpOp op;

protected:
    OpExp(ExpOp op) : op {op} {}

    OpExp(const OpExp&) = delete;
    void operator=(const OpExp &) = delete;

public:
    /// Acceptor
    virtual void accept(Visitor& visit) override;

    ExpOp get_operator() {
        return (op);
    }

};

class BinOpExp : public OpExp {
private:
    shared_ptr<Exp> left;
    shared_ptr<Exp> right;
public:
    BinOpExp(ExpOp op, shared_ptr<Exp> left, shared_ptr<Exp> right) :
        OpExp(op), left {left}, right {right} {}

    /// Create a expression that represents the
    /// conjuction of the given arguments
    static shared_ptr<Exp> make_andd(shared_ptr<Exp> exp1,
                                     shared_ptr<Exp> exp2) {
        return make_shared<BinOpExp>(ExpOp::andd, exp1, exp2);
    }

    shared_ptr<Exp> get_first_argument() {
        return (left);
    }

    shared_ptr<Exp> get_second_argument() {
        return (right);
    }

    virtual void accept(Visitor& visit) override;
};

class UnOpExp : public OpExp {
private:
    shared_ptr<Exp> argument;
public:
    UnOpExp(ExpOp op, shared_ptr<Exp> argument) :
       OpExp(op), argument {argument} {}

    shared_ptr<Exp> get_argument() {
        return (argument);
    }

    /// Create a expression that represents the negation of the given argument
    static shared_ptr<Exp> make_nott(shared_ptr<Exp> exp) {
        return make_shared<UnOpExp>(ExpOp::nott, exp);
    }

    virtual void accept(Visitor& visit) override;
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
    virtual void visit(shared_ptr<ModelAST>);

    //Model
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<Model>);

    // Module
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<ModuleAST>);

    // Properties
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<Prop>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<TransientProp>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<RateProp>);

    // Declarations
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<Decl>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<InitializedDecl>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<RangedDecl>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<ClockDecl>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<ArrayDecl>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<InitializedArray>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<MultipleInitializedArray>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<RangedInitializedArray>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<RangedMultipleInitializedArray>);

    //Transitions
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<TransitionAST>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<OutputTransition>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<TauTransition>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<InputTransition>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<InputCommittedTransition>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<OutputCommittedTransition>);

    //Effects
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<Effect>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<Assignment>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<ClockReset>);

    //Distributions
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<Dist>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<SingleParameterDist>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<MultipleParameterDist>);

    //Locations
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<Location>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<ArrayPosition>);

    //Expressions
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<Exp>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<IConst>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<BConst>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<FConst>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<LocExp>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<OpExp>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<BinOpExp>);
    /// @copydoc visit(shared_ptr<ModelAST>)
    virtual void visit(shared_ptr<UnOpExp>);
};

#endif

