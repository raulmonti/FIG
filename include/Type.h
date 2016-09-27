/* Leonardo Rodríguez */
#ifndef  TYPE_H
#define  TYPE_H

#include <cassert>
#include <memory>
#include <vector>
#include "Util.h"

/// @brief Types for local module variables.
enum class Type {tint, tbool, tfloat, tclock, tunknown};

class BasicTy;
class FunTy;
class UnaryOpTy;
class BinaryOpTy;

class Ty {
protected:
    Ty() {}
public:
    virtual bool is_basic() const {
        return (false);
    }

    virtual bool is_fun() const {
        return (false);
    }

    virtual bool is_binary_type() const {
        return (false);
    }

    virtual bool is_unary_type() const {
        return (false);
    }

    BasicTy to_basic() const;
    FunTy to_fun() const;
    UnaryOpTy to_unary_ty() const;
    BinaryOpTy to_binary_ty() const;

    virtual std::string to_string() const {
        return "?"; //lot of errors when pure virtual.
    }

    static std::string to_string(Type type) {
        std::string result;
        switch(type) {
        case Type::tint:
            result = "Int";
            break;
        case Type::tbool:
            result = "Bool";
            break;
        case Type::tfloat:
            result = "Float";
            break;
        case Type::tclock:
            result = "Clock";
            break;
        case Type::tunknown:
            result = "[?]";
            break;
        }
        return result;
    }
};

//Type equality
bool operator==(const Ty& ty1, const Ty& ty2);

//Subtype relation
bool operator<=(const Ty& ty1, const Ty& ty2);

//Strict subtype relation
bool operator<(const Ty& ty1, const Ty& ty2);

class BasicTy : public Ty {
private:
    Type type;
public:
    BasicTy(Type type) : type {type} {}

    bool is_basic() const override {
        return (true);
    }

    Type get_type() const {
        return (type);
    }

    virtual std::string to_string() const override;
};

class Unknown : public BasicTy {
public:
    Unknown() : BasicTy(Type::tunknown) {}
};

class FunTy : public Ty {
private:
    std::shared_ptr<Ty> ty1;
    std::shared_ptr<Ty> ty2;

public:
    FunTy(std::shared_ptr<Ty> ty1, std::shared_ptr<Ty> ty2):
        ty1 {ty1}, ty2 {ty2} {}

    FunTy(Type type1, Type type2) {
        ty1 = std::make_shared<BasicTy>(type1);
        ty2 = std::make_shared<BasicTy>(type2);
    }

    std::shared_ptr<Ty> get_ty1() const {
        return (ty1);
    }

    std::shared_ptr<Ty> get_ty2() const {
        return (ty2);
    }

    bool is_fun() const override {
        return (true);
    }

    virtual std::string to_string() const override;
};

class UnaryOpTy : public FunTy {
public:
    UnaryOpTy(Type argtype, Type result) :
        FunTy (argtype, result) {}

    Type get_arg_type() const {
        return get_ty1()->to_basic().get_type();
    }

    Type get_result_type() const {
        return get_ty2()->to_basic().get_type();
    }

    bool is_unary_type() const override {
        return (true);
    }
};

class BinaryOpTy : public FunTy {
public:
    BinaryOpTy(Type a1type, Type a2type, Type result) :
        FunTy (std::make_shared<BasicTy>(a1type),
               std::make_shared<FunTy>(a2type, result)) {}

    Type get_arg1_type() const {
        return  get_ty1()->to_basic().get_type();
    }

    Type get_arg2_type() const {
        FunTy snd = get_ty2()->to_fun();
        return (snd.get_ty1()->to_basic().get_type());
    }

    Type get_result_type() const {
        FunTy snd = get_ty2()->to_fun();
        return (snd.get_ty2()->to_basic().get_type());
    }

    bool is_binary_type() const override {
        return (true);
    }
};

#endif
