/* Leonardo Rodríguez */

#ifndef MODEL_TC_H
#define MODEL_TC_H

#include "ModelAST.h"
#include "Util.h"
#include <utility>
#include <unordered_map>

using std::string;
using std::unordered_multimap;

/**
 * @brief A symbol table for a module. Contains the
 * module clocks, the local declarations, and more. Built
 * during type-checking.
 */
struct ModuleScope {
    /// Static map that store the symbol table for each module
    /// indexed by the module's name
    static shared_map<string, ModuleScope> scopes;

    /// Global constants (name->declaration)
    static shared_map<string, Decl> globals;

    /// The name of this module
    string id;

    /// The module itself
    shared_ptr<ModuleAST> body;

    /// Mapping each label with its type
    map<string, LabelType> labels;

    /// Mapping labels with its transition*s*
    unordered_multimap<string, shared_ptr<TransitionAST>> label_transitions;

    /// Mapping each clock with its distribution
    // @todo redesign this when implementing clock arrays
    shared_map<string, Dist> clock_dists;

    /// Mapping each identifier with its declaration
    shared_map<string, Decl> local_decls;

    /// Mapping transitions triggered by a clock.
    unordered_multimap<string, shared_ptr<OutputTransition>>
    triggered_transitions;

    /// Find an identifier in every module. Mainly used to build
    /// properties, since they can contain variables of any module.
    static shared_ptr<Decl> find_in_all_modules(const string &id) {
        shared_ptr<Decl> result = nullptr;
        for (auto entry : ModuleScope::scopes) {
            const shared_ptr<ModuleScope> &curr = entry.second;
            const shared_map<string, Decl> &local = curr->local_decls;
            if (local.find(id) != local.end()) {
                result = local.at(id);
                break;
            }
        }
        return (result);
    }

    /// Check if all the modules have less transitions than the given bound
    static bool modules_size_bounded_by(unsigned long int bound) {
        bool result = true;
        for (auto entry : ModuleScope::scopes) {
            const shared_ptr<ModuleScope> &curr = entry.second;
            auto &transitions = curr->body->get_transitions();
            result = result && (transitions.size() <= bound);
        }
        return (result);
    }
};

/**
 * @brief A visitor to typecheck the AST. It will traverse the
 * AST finding type-errors, identifiers out of scope, duplicate
 * identifiers, etc.
 */
class ModelTC : public Visitor {
private:
    /// Shortcut for the static ModuleScope member.
    shared_map<string, ModuleScope> &scopes  = ModuleScope::scopes;

    /// Shortcut for the map of global variables.
    shared_map<string, Decl> &globals = ModuleScope::globals;

    /// The current module scope.
    // when traversing the AST it is necessary to keep track
    // of the current module scope to find idenfiers declarations
    shared_ptr<ModuleScope> current_scope;

    /// The type of the last node visited.
    Type last_type;

    /// Is this visitor checking a property?
    /// This allows identifiers of expressions to occur on any module
    /// and not just in the current one.
    bool checking_property = false;

    /// Accepts if no errors
    void accept_cond(shared_ptr<ModelAST> module);

    /// If the last inferred type is not "type", put an error
    void check_type(Type type, const string &msg);

    /// Check if every clock has a distribution type
    void check_clocks(shared_ptr<ModuleScope> scope);

    /// Check if the given expression is in DNF
    void check_dnf(PropType type, shared_ptr<Exp> exp);

    /// Find the type of the given identifier
    Type identifier_type(const string &id);

    /// Checking the global declarations (constants)
    bool is_global_scope() {
        return (current_scope == nullptr);
    }

    /// Type of the result of an operator application, given the
    /// type of its arguments.
    static Type operator_type(const ExpOp &id, Type arg);

    /// Check range and initialization of the declaration
    void check_ranged_decl(shared_ptr<RangedDecl> decl);
    void check_ranged_all(shared_ptr<ModuleScope> scope);

    /// Check if parameters of distributions are reducible
    void check_dist(shared_ptr<Dist> dist);
    void check_dist(shared_ptr<ModuleScope> scope);

    void check_scope(shared_ptr<Decl> decl);

    /// Used to evaluate range bounds and distributions parameters
    int eval_int_or_put(shared_ptr<Exp> exp);
    float eval_float_or_put(shared_ptr<Exp> exp);
public:
    ModelTC() : current_scope {nullptr},
        last_type {Type::tunknown},
        checking_property {false} {};
    virtual ~ModelTC();
    /// Visitor functions
    void visit(shared_ptr<Model> node) override;
    void visit(shared_ptr<ModuleAST> node) override;
    void visit(shared_ptr<RangedDecl> node) override;
    void visit(shared_ptr<InitializedDecl> node) override;
    void visit(shared_ptr<ArrayDecl> node) override;
    void visit(shared_ptr<ClockDecl> node) override;
    void visit(shared_ptr<TransitionAST> node) override;
    void visit(shared_ptr<Assignment> node) override;
    void visit(shared_ptr<ClockReset> node) override;
    void visit(shared_ptr<MultipleParameterDist> node) override;
    void visit(shared_ptr<SingleParameterDist> node) override;
    void visit(shared_ptr<Location> node) override;
    void visit(shared_ptr<ArrayPosition> node) override;
    void visit(shared_ptr<IConst> node) override;
    void visit(shared_ptr<BConst> node) override;
    void visit(shared_ptr<FConst> node) override;
    void visit(shared_ptr<LocExp> node) override;
    void visit(shared_ptr<BinOpExp> node) override;
    void visit(shared_ptr<UnOpExp> node) override;
    void visit(shared_ptr<TransientProp> node) override;
    void visit(shared_ptr<RateProp> node) override;
};

#endif
