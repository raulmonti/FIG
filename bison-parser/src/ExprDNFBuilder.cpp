#include "ExprDNFBuilder.h"

void ExprClauseBuilder::visit(shared_ptr<IConst> iconst) {
    (void) iconst;
    //do nothing!
}

void ExprClauseBuilder::visit(shared_ptr<BConst> bconst) {
    clause.push_back(bconst);
}

void ExprClauseBuilder::visit(shared_ptr<FConst> node) {
     (void) node;
     //do nothing!
}

void ExprClauseBuilder::visit(shared_ptr<LocExp> exp) {
    if (exp->type == Type::tbool) {
	clause.push_back(exp);
    }	    
}

void ExprClauseBuilder::visit(shared_ptr<OpExp> exp) {
    switch (exp->bop) {
    case ExpOp::andd:
    {
	//look for the clauses in both sides
	ExprClauseBuilder left_b;
	exp->left->accept(left_b);
	ExprClauseBuilder right_b;
	exp->right->accept(right_b);
	break;
    }
    case ExpOp::orr:
    {
	put_error("Not a proper clause - Property not in DNF");
	break;
    }
    default:
    {
	//the expression itself considered a clause
	clause.push_back(exp);
    }
    }
}

void ExprDNFBuilder::visit(shared_ptr<IConst> iconst) {
    (void) iconst;
    //do nothing!
}

void ExprDNFBuilder::visit(shared_ptr<BConst> bconst) {
    //clause with a single term
    clause_vector.push_back(vector<shared_ptr<Exp>> {bconst});
}

void ExprDNFBuilder::visit(shared_ptr<FConst> node) {
     (void) node;
     //do nothing!
}

void ExprDNFBuilder::visit(shared_ptr<LocExp> exp) {
    if (exp->type == Type::tbool) {
	clause_vector.push_back(vector<shared_ptr<Exp>> {exp});
    }	    
}

void ExprDNFBuilder::visit(shared_ptr<OpExp> exp) {
    switch (exp->bop) {
    case ExpOp::orr:
    {
	ExprClauseBuilder left_b;
	exp->left->accept(left_b);
	if (left_b.has_errors()) {
	    //not a clause, then it must be a dnf
	    exp->left->accept(*this);
	} else {
	    clause_vector.push_back(left_b.get_clause());
	}
	ExprClauseBuilder right_b;
	exp->right->accept(right_b);
	if (right_b.has_errors()) {
	    exp->right->accept(*this);
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
	clause_vector.push_back(vector<shared_ptr<Exp>> {exp});
    }
    }
}


