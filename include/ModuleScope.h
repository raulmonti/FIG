/* Leonardo Rodríguez */
#ifndef MODULE_SCOPE_H
#define MODULE_SCOPE_H

#include "ModelAST.h"
#include "FigException.h"
#include <unordered_map>
#include <thread>
#include <mutex>

using std::string;
using std::unordered_multimap;

using triggered_map  = unordered_multimap<string, shared_ptr<OutputTransition>>;
using transition_map = unordered_multimap<string, shared_ptr<TransitionAST>>;

/**
 * @brief A symbol table for a module. Contains the
 * module clocks, the local declarations, and more. Built
 * during type-checking.
 */
class ModuleScope {
public:
	inline virtual ~ModuleScope() { clear(); }

public:
    /// Static map that store the symbol table for each module
    /// indexed by the module's name
    static shared_map<string, ModuleScope> scopes;

    /// Global constants (name->declaration)
    static shared_map<string, Decl> globals;

protected:
	/// Delete all information about model/modules scopes,
	/// @note These fields are deleted JIT by the \ref ModelTC "model type
	///       checker", in the function ModelTC::visit(shared_ptr<Model>),
	///       before a new model file is parsed and checked.
	static void clear() {
		ModuleScope::scopes.clear();
		ModuleScope::globals.clear();
	}

protected:
    /// The name of this module
    string id;

    /// The module itself
    shared_ptr<ModuleAST> body;

    /// Mapping each label with its type
    map<string, LabelType> labels;

    /// Mapping labels with its transition*s*
    transition_map label_transitions;

    /// Mapping each clock with its distribution
    // @todo redesign this when implementing clock arrays
    shared_map<string, Dist> clock_dists;

    /// Mapping each identifier with its declaration
    shared_map<string, Decl> local_decls;

    /// Mapping transitions triggered by a clock.
    triggered_map triggered_transitions;

public:
    virtual string get_module_name() {
        return (id);
    }

    virtual void set_module_name(const string &id) {
        this->id = id;
    }

    virtual shared_ptr<ModuleAST> module_ast() {
        return (body);
    }

    virtual void set_module_ast(shared_ptr<ModuleAST> ast) {
        this->body = ast;
    }

    virtual shared_map<string, Decl>& local_decls_map() {
        return (local_decls);
    }

    virtual map<string, LabelType>& type_by_label_map() {
        return (labels);
    }

    virtual triggered_map& transition_by_clock_map() {
        return (triggered_transitions);
    }

    virtual transition_map& transition_by_label_map() {
        return (label_transitions);
    }

    virtual shared_map<string, Dist>& dist_by_clock_map() {
        return (clock_dists);
    }

public:
    /// Find identifier declaration in global scope.
    static shared_ptr<Decl> find_identifier_global(const string &id) {
        shared_ptr<Decl> decl = nullptr;
        if (ModuleScope::globals.find(id) != ModuleScope::globals.end()) {
            decl = ModuleScope::globals.at(id);
        }
        return (decl);
    }

    /// Find an identifier in this module scope. If it is not declared locally,
    /// the global scope is searched.
    shared_ptr<Decl> find_identifier(const string &id) {
        //try to find the identifier, first locally then globally
        shared_ptr<Decl> decl = nullptr;
        if (local_decls.find(id) != local_decls.end()) {
            decl = local_decls.at(id);
        } else {
            decl = ModuleScope::find_identifier_global(id);
        }
        return (decl);
    }

    /// Find an identifier in the given scope. If scope is null, the
    /// global scope is searched
    static shared_ptr<Decl> find_identifier_on(shared_ptr<ModuleScope> scope,
                                                const string &id) {
        return scope == nullptr ?
                    find_identifier_global(id) : scope->find_identifier(id);

    }


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

class CompositeModuleScope : public ModuleScope {
private:
    static shared_ptr<CompositeModuleScope> instance_;
    static std::once_flag singleInstance_;
    //Private constructor
    CompositeModuleScope() {build_scope();}

    void build_scope() {
        for (auto entry : ModuleScope::scopes) {
            shared_ptr<ModuleScope> current = entry.second;
            const shared_map<string, Decl> &local_map
                    = current->local_decls_map();
            for (auto id_decl : local_map) {
                this->local_decls[id_decl.first] = id_decl.second;
            }
        }
    }

public:
    static inline shared_ptr<CompositeModuleScope> get_instance() {
        std::call_once(singleInstance_,
					   [] () { instance_.reset(new CompositeModuleScope);});
        return instance_;
	}

	virtual ~CompositeModuleScope() { clear(); }

	/// Delete all information about model/modules scopes,
	/// <b>including the one that resides in static class members</b>
	inline void clear() {
		ModuleScope::clear();
		id.clear();
		body.reset();
		labels.clear();
		label_transitions.clear();
		clock_dists.clear();
		local_decls.clear();
		triggered_transitions.clear();
	}

    string get_module_name() override {
        throw_FigException("Not implemented");
    }

    shared_ptr<ModuleAST> module_ast() override {
        throw_FigException("Not implemented");
    }

    shared_map<string, Decl>& local_decls_map() override {
        return (local_decls);
    }

    map<string, LabelType>& type_by_label_map() override {
        throw_FigException("Not implemented");
    }

    triggered_map& transition_by_clock_map() override {
        throw_FigException("Not implemented");
    }

    transition_map& transition_by_label_map() override {
        throw_FigException("Not implemented");
    }

    shared_map<string, Dist>& dist_by_clock_map() override {
        throw_FigException("Not implemented");
    }

};

#endif
