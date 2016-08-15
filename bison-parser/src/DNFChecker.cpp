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
    _dnf = (exp->type == Type::tbool);
}

void DNFChecker::visit(shared_ptr<OpExp> exp) {
    switch(exp->bop) {
    case ExpOp::orr:
    {
	assert (exp->arity == Arity::two);
	//is in dnf is both expressions are OR...
	exp->left->accept(*this);
	bool left_dnf = _dnf;
	exp->right->accept(*this);
	bool right_dnf = _dnf;
	//each term is a clause
	if (!left_dnf) {
	    //check if it is a clause
	    ClauseChecker left_c;
	    exp->left->accept(left_c);
	    left_dnf = left_c.is_clause();
	}
	if (!right_dnf) {
		//check if it is a clause
	    ClauseChecker right_c;
	    exp->right->accept(right_c);
	    right_dnf = right_c.is_clause();	
	}
	_dnf = (left_dnf && right_dnf);
	break;
    }
    case ExpOp::andd:
    {
	assert (exp->arity == Arity::two);
	_dnf = false;
	break;
    }
    default:
    {
	_dnf = (exp->type == Type::tbool);
    }
    }
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
    _clause = (exp->type == Type::tbool);
}

void ClauseChecker::visit(shared_ptr<OpExp> exp) {
    switch(exp->bop) {
    case ExpOp::orr:
    {
	_clause = false;
	break;
    }
    case ExpOp::andd:
    {
	exp->left->accept(*this);
	bool left_clause = _clause;
	exp->right->accept(*this);
	bool right_clause = _clause;
	_clause = (left_clause && right_clause);
	break;
    }
    default:
    {
	_clause = (exp->type == Type::tbool);
    }
    }
}
