/* Leonardo Rodríguez */

#include "DNFChecker.h"
#include "ModelPrinter.h"

void DNFChecker::visit(shared_ptr<IConst> iconst) {
    (void) iconst;
    _dnf = false;
}

void DNFChecker::visit(shared_ptr<BConst> bconst) {
    (void) bconst;
    _dnf = true;
}

void DNFChecker::visit(shared_ptr<FConst> fconst) {
    (void) fconst;
    _dnf = false;
}

void DNFChecker::visit(shared_ptr<LocExp> exp) {
    _dnf = (exp->get_type() == Type::tbool);
}

void DNFChecker::visit(shared_ptr<BinOpExp> exp) {
    switch(exp->get_operator()) {
    case ExpOp::orr:
    {
        //is in dnf is both expressions are, OR...
        exp->get_first_argument()->accept(*this);
        bool left_dnf = _dnf;
        exp->get_second_argument()->accept(*this);
        bool right_dnf = _dnf;
        //... each term is a clause
        if (!left_dnf) {
            //check if it is a clause
            ClauseChecker left_c;
            exp->get_first_argument()->accept(left_c);
            left_dnf = left_c.is_clause();
        }
        if (!right_dnf) {
            //check if it is a clause
            ClauseChecker right_c;
            exp->get_second_argument()->accept(right_c);
            right_dnf = right_c.is_clause();
        }
        _dnf = (left_dnf && right_dnf);
        break;
    }
    case ExpOp::andd:
    {
        //cannot be in dnf unless it is a single clause
        ClauseChecker clause;
        exp->accept(clause);
        _dnf = clause.is_clause();
        break;
    }
    default:
    {
        _dnf = (exp->get_type() == Type::tbool);
    }
    }
}

void DNFChecker::visit(shared_ptr<UnOpExp> exp) {
    _dnf = (exp->get_type() == Type::tbool);
}

void ClauseChecker::visit(shared_ptr<IConst> iconst) {
    (void) iconst;
    _clause = false;
}

void ClauseChecker::visit(shared_ptr<BConst> bconst) {
    (void) bconst;
    _clause = true;
}

void ClauseChecker::visit(shared_ptr<FConst> fconst) {
    (void) fconst;
    _clause = false;
}

void ClauseChecker::visit(shared_ptr<LocExp> exp) {
    _clause = (exp->get_type() == Type::tbool);
}

void ClauseChecker::visit(shared_ptr<BinOpExp> exp) {
    switch(exp->get_operator()) {
    case ExpOp::orr:
    {
        _clause = false;
        break;
    }
    case ExpOp::andd:
    {
        exp->get_first_argument()->accept(*this);
        bool left_clause = _clause;
        exp->get_second_argument()->accept(*this);
        bool right_clause = _clause;
        _clause = (left_clause && right_clause);
        break;
    }
    default:
    {
        _clause = (exp->get_type() == Type::tbool);
    }
    }
}

void ClauseChecker::visit(shared_ptr<UnOpExp> exp) {
    _clause = (exp->get_type() == Type::tbool);
}
