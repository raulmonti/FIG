#include "ModelTC.h"
#include <functional>

/* This visitor reduces expressions to constants.
 * The expressions that can be reduced in compilation time
 * do not depend on state variables, only on global constans
 * and initializations.
 */

class ExpEvaluator : public Visitor {
private:
    map<string, Decl*>& globals = ModuleScope::globals;
    Type type;
    union value_holder_t {
	bool bvalue;
	int ivalue;
	float fvalue;
    } value;
    void reduce_unary_operator(OpExp *exp);
    void reduce_binary_operator(OpExp *exp);
    void mark_not_reducible();

    template<typename T>
    static std::function<T (T)> uop_as_fun(ExpOp op);
    template<typename T>
    static std::function<T (T, T)> bop_as_fun(ExpOp op);
    template<typename T>
    static std::function<bool (T, T)> bop_as_rel(ExpOp op);

public:
    ExpEvaluator() : type {Type::tunknown} {};
    bool was_reduced();
    int get_int();
    bool get_bool();
    float get_float();
    bool has_type_int();
    bool has_type_bool();
    bool has_type_float();
    //set ready to use again
    void reset();
    void visit(IConst* node);
    void visit(BConst* node);
    void visit(FConst* node);
    void visit(LocExp* node);
    void visit(OpExp* node);
};
