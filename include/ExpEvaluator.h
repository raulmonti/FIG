/* Leonardo Rodríguez */

#ifndef EXPEVALUATOR_H
#define EXPEVALUATOR_H

#include "ModelTC.h"
#include <functional>

using std::shared_ptr;
using std::make_shared;

/* This visitor reduces expressions to constants.
 * The expressions that can be reduced in compilation time
 * do not depend on state variables, only on global constans
 * and initializations.
 */

class ExpEvaluator : public Visitor {
private:
    shared_map<string, Decl>& globals = ModuleScope::globals;
    Type type;
    union value_holder_t {
	bool bvalue;
	int ivalue;
	float fvalue;
    } value;
    void reduce_unary_operator(shared_ptr<OpExp> exp);
    void reduce_binary_operator(shared_ptr<OpExp> exp);
    void mark_not_reducible();
public:
    ExpEvaluator() : type {Type::tunknown} {};
    template<typename T>
    static std::function<T (T)> uop_as_fun(ExpOp op);
    template<typename T>
    static std::function<T (T, T)> bop_as_fun(ExpOp op);
    template<typename T>
    static std::function<bool (T, T)> bop_as_rel(ExpOp op);
    bool was_reduced();
    int get_int();
    bool get_bool();
    float get_float();
    bool has_type_int();
    bool has_type_bool();
    bool has_type_float();
    //set ready to use again
    void reset();
    void visit(shared_ptr<IConst> node);
    void visit(shared_ptr<BConst> node);
    void visit(shared_ptr<FConst> node);
    void visit(shared_ptr<LocExp> node);
    void visit(shared_ptr<OpExp> node);
};

#endif
