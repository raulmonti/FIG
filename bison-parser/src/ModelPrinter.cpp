#include "ModelPrinter.h"
#include <string>

using namespace std;

string ModelPrinter::to_str(Type type) {
    string result;
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
        result = "Unknown";
        break;
    }
    return result;
}

string ModelPrinter::to_str(LabelType type) {
    string result;
    switch(type) {
    case LabelType::in:
        result = "Input";
        break;
    case LabelType::out:
        result = "Output";
        break;
    case LabelType::commited:
        result = "Commited";
        break;
    case LabelType::empty:
        result = "None";
        break;
    }
    return result;
}

string ModelPrinter::to_str(DistType type) {
    string result;
    switch(type) {
    case DistType::erlang:
        result = "erlang";
        break;
    case DistType::uniform:
        result = "uniform";
        break;
    case DistType::normal:
        result = "normal";
        break;
    case DistType::exponential:
        result = "exponential";
        break;
    }
    return result;
}

string ModelPrinter::to_str(ExpOp op) {
    string result;
    switch(op) {
    case ExpOp::plus: result = "(+)"; break;
    case ExpOp::times: result = "(*)"; break;
    case ExpOp::minus: result = "(-)"; break;
    case ExpOp::div: result = "(/)"; break;
    case ExpOp::mod: result = "(%)"; break;
    case ExpOp::andd: result = "(&)"; break;
    case ExpOp::orr: result = "(|)"; break;
    case ExpOp::nott: result = "(!)"; break;
    case ExpOp::eq: result = "(==)"; break;
    case ExpOp::neq: result = "(!=)"; break;
    case ExpOp::lt: result = "(<)"; break;
    case ExpOp::gt: result = "(>)"; break;
    case ExpOp::le: result = "(<=)"; break;
    case ExpOp::ge: result = "(>=)"; break;
    }
    return result;
}

void ModelPrinter::accept_idented(ModelAST *node) {
    ident++;
    node->accept(*this);
    ident--;
}

void ModelPrinter::print_idented(string str) {
    cout << string(ident, '\t') << str << endl;
}

void ModelPrinter::visit(ModelAST *ast) {
    (void) ast;
    print_idented("Node:");
}

void ModelPrinter::visit(Model* model) {
    print_idented("=Model=");
    print_idented("Global constants:");
    for (auto decl : model->globals) {
        accept_idented(decl);
    }
    print_idented("Modules:");
    for (auto body : model->modules) {
        print_idented("Module: " + body.first);
        accept_idented(body.second);
    }
}

void ModelPrinter::visit(ModuleBody *body) {
    print_idented("=ModuleBody=");
    print_idented("Local Declarations: ");
    for (auto decl : body->local_decls) {
        accept_idented(decl);
    }
    print_idented("Actions: ");
    for (auto action : body->actions) {
        accept_idented(action);
    }
}

void ModelPrinter::visit(Decl* decl) {
    print_idented("=Decl=");
    print_idented("ID: " + decl->id);
    print_idented("Type : " + to_str(decl->type));
    if (decl->has_range()) {
        print_idented("Range:");
        print_idented("Lower:");
        accept_idented(decl->lower);
        print_idented("Upper:");
        accept_idented(decl->upper);
    }
    if (decl->is_array()) {
        print_idented("Array Size:");
        accept_idented(decl->size);
    }
    if (decl->inits.size() > 0) {
        print_idented("Init:");
        for (auto init : decl->inits) {
            accept_idented(init);
        }
    }
}

void ModelPrinter::visit(Action* action) {
    print_idented("=Action=");
    print_idented("Label: " + action->id);
    print_idented("Label Type: " + to_str(action->type));
    print_idented("Guard:");
    accept_idented(action->guard);
    if (action->has_clock()) {
        print_idented("Clock Location:");
        accept_idented(action->clock_loc);
    }
    print_idented("Effects:");
    for (auto effect : action->effects) {
        accept_idented(effect);
    }
}

void ModelPrinter::visit(Effect* effect) {
    print_idented("=Effect=");
    print_idented("Location:");
    accept_idented(effect->loc);
    if (effect->is_clock_reset()) {
        print_idented("Clock Reset:");
        print_idented("Dist:");
        accept_idented(effect->dist);
    }
    if (effect->is_state_change()) {
        print_idented("State Change:");
        accept_idented(effect->arg);
    }
}

void ModelPrinter::visit(Dist *dist) {
    print_idented("=Dist=");
    print_idented("Type: " + to_str(dist->type));
    if (dist->arity == Arity::one) {
        print_idented("Param1:");
        accept_idented(dist->param1);
    } else if (dist->arity == Arity::two) {
        print_idented("Param1:");
        accept_idented(dist->param1);
        print_idented("Param2:");
        accept_idented(dist->param2);
    }
}

void ModelPrinter::visit(Location *loc) {
    print_idented("=Location=");
    print_idented("ID: \"" + loc->id + "\"");
    if (loc->is_array_position()) {
        print_idented("Array Position:");
        accept_idented(loc->index);
    }
}

void ModelPrinter::visit(Exp *exp) {
    (void) exp;
    print_idented("=Exp=");
}

void ModelPrinter::visit(IConst *node) {
    print_idented("Int Value: " + std::to_string(node->value));
}

void ModelPrinter::visit(BConst *node) {
    print_idented("Bool Value: " + node->value ? "true" : "false");
}

void ModelPrinter::visit(FConst *node) {
    print_idented("Float Value: " + std::to_string(node->value));
}

void ModelPrinter::visit(LocExp *node) {
    print_idented("Value Of");
    accept_idented(node->location);
}

void ModelPrinter::visit(OpExp *node) {
    print_idented("Operator: " + to_str(node->bop));
    if (node->arity == Arity::one) {
        accept_idented(node->left);
    } else if (node->arity == Arity::two) {
        accept_idented(node->left);
        accept_idented(node->right);
    }
}
