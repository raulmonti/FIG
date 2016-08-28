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
    shared_ptr<ModuleBody> body;

    /// Mapping each label with its type
    map<string, LabelType> labels;

    /// Mapping labels with its transition*s*
    unordered_multimap<string, shared_ptr<Action>> label_actions;

    /// Mapping each clock with its distribution
    // @todo redesign this when implementing clock arrays
    shared_map<string, Dist> clock_dists;

    /// Mapping each identifier with its declaration
    shared_map<string, Decl> local_decls;

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

    //accepts if no errors
    void accept_cond(shared_ptr<ModelAST> module);
    //prefix for log message
    void check_type(Type type, const string &msg);
    void check_clocks(shared_ptr<ModuleScope> scope);
    void check_dnf(PropType type, shared_ptr<Exp> exp);
    Type identifier_type(const string &id);
    bool is_global_scope() {
        return (current_scope == nullptr);
    }
    // result type of a operator given the type of its arguments
    // may be Type::tunknown
    static Type operator_type(const ExpOp &id, Type arg);
public:
    ModelTC() : current_scope {nullptr},
        last_type {Type::tunknown},
        checking_property {false} {};
    virtual ~ModelTC();
    void visit(shared_ptr<Model> node);
    void visit(shared_ptr<ModuleBody> node);
    void visit(shared_ptr<Decl> node);
    void visit(shared_ptr<Action> node);
    void visit(shared_ptr<Effect> node);
    void visit(shared_ptr<Dist> node);
    void visit(shared_ptr<Location> node);
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
    void visit(shared_ptr<Prop> node);
};

#endif
