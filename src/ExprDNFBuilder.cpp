/* Leonardo Rodríguez */
#include "ExprDNFBuilder.h"
#include "ExpReductor.h"
#include "FigException.h"

namespace {
shared_ptr<Exp> reduce_exp(shared_ptr<Exp> exp) {
    shared_ptr<Exp> result = nullptr;
    ExpReductor red;
    exp->accept(red);
    result = red.get_reduced_exp();
    return (result);
}
}

void ExprClauseBuilder::visit(shared_ptr<IConst> iconst) {
    (void) iconst;
    throw_FigException("Not a boolean expression");
}

void ExprClauseBuilder::visit(shared_ptr<BConst> bconst) {
    clause.push_back(bconst);
}

void ExprClauseBuilder::visit(shared_ptr<FConst> fconst) {
    (void) fconst;
   throw_FigException("Not a boolean expression");
}

void ExprClauseBuilder::visit(shared_ptr<LocExp> exp) {
    if (exp->get_type() == Type::tbool) {
        clause.push_back(::reduce_exp(exp));
    } else {
        throw_FigException("Not a boolean expression");
    }
}

void ExprClauseBuilder::visit(shared_ptr<BinOpExp> exp) {
    switch (exp->get_operator()) {
    case ExpOp::andd:
    {
        //look for the clauses in both sides
        exp->get_first_argument()->accept(*this);
        exp->get_second_argument()->accept(*this);
        break;
    }
    case ExpOp::orr:
    {
        put_error("Not a proper clause - Property not in DNF");
        break;
    }
    default:
    {
        //the expression itself (reduced if possible) considered a clause
        clause.push_back(::reduce_exp(exp));
    }
    }
}

void ExprClauseBuilder::visit(shared_ptr<UnOpExp> exp) {
    clause.push_back(::reduce_exp(exp));
}


void ExprDNFBuilder::visit(shared_ptr<IConst> iconst) {
    (void) iconst;
    throw_FigException("Not a boolean expression");
}

void ExprDNFBuilder::visit(shared_ptr<BConst> bconst) {
    //clause with a single term
    clause_vector.push_back(vector<shared_ptr<Exp>> {bconst});
}

void ExprDNFBuilder::visit(shared_ptr<FConst> node) {
    (void) node;
    throw_FigException("Not a boolean expression");
}

void ExprDNFBuilder::visit(shared_ptr<LocExp> exp) {
    if (exp->get_type() == Type::tbool) {
        clause_vector.push_back(vector<shared_ptr<Exp>>{::reduce_exp(exp)});
    } else {
        throw_FigException("Not a boolean expression");
    }
}

void ExprDNFBuilder::visit(shared_ptr<BinOpExp> exp) {
    switch (exp->get_operator()) {
    case ExpOp::orr:
    {
        ExprClauseBuilder left_b;
        exp->get_first_argument()->accept(left_b);
        if (left_b.has_errors()) {
            //not a clause, then it must be a dnf
            exp->get_first_argument()->accept(*this);
        } else {
            clause_vector.push_back(left_b.get_clause());
        }
        ExprClauseBuilder right_b;
        exp->get_second_argument()->accept(right_b);
        if (right_b.has_errors()) {
            exp->get_second_argument()->accept(*this);
        } else {
            clause_vector.push_back(right_b.get_clause());
        }
        break;
    }
    case ExpOp::andd:
    {
        put_error("Property not in DNF");
        break;
    }
    default:
    {
        //the expression itself considered a clause
        clause_vector.push_back(vector<shared_ptr<Exp>>{::reduce_exp(exp)});
    }
    }
}

void ExprDNFBuilder::visit(shared_ptr<UnOpExp> exp) {
    clause_vector.push_back(vector<shared_ptr<Exp>>{::reduce_exp(exp)});
}
