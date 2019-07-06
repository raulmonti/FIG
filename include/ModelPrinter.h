//==============================================================================
//
//  ModelPrinter.h
//
//  Copyright 2016-
//  Authors:
//  - Leonardo Rodríguez (Universidad Nacional de Córdoba)
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
//
//------------------------------------------------------------------------------
//
//  This file is part of FIG.
//
//  The Finite Improbability Generator (FIG) project is free software;
//  you can redistribute it and/or modify it under the terms of the GNU
//  General Public License as published by the Free Software Foundation;
//  either version 3 of the License, or (at your option) any later version.
//
//  FIG is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with FIG; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================

#ifndef MODEL_PRINTER_H
#define MODEL_PRINTER_H

#include <iostream>
#include "ModelAST.h"


/** Visitor to print the Model's AST **/
class ModelPrinter : public Visitor {
	/// Print for debugging?  (won't be parseable!)
	const bool debug;
	/// Stream where prints will be dumped
	std::ostream& out;
	/// Current indentation level to use
	unsigned ident;
	/// Charater used for identation
	const char identc;
	/// Is the current declaration a constant value?
	bool constant;
	/// Dump to out stream with currently set indentation level
	void print_indented(string str);
	/// Print this module using one more level of indentation
	void accept_indented(std::shared_ptr<ModelAST> node);
public:
	ModelPrinter(std::ostream& sout = std::cout,
				 bool debugp = false,
				 const char& identChar = '\t') :
		debug(debugp),
		out(sout),
		ident(0),
		identc(identChar),
		constant(false)
	{ /* Not much to do around here */ }

	virtual ~ModelPrinter() {}

	void visit(std::shared_ptr<Model> node);
	void visit(std::shared_ptr<ModuleAST> node);
	void visit(std::shared_ptr<Decl> node);
	void visit(std::shared_ptr<RangedDecl> node);
	void visit(std::shared_ptr<InitializedDecl> node);
	void visit(std::shared_ptr<ClockDecl> node);
	void visit(std::shared_ptr<ArrayDecl> node);
	void visit(std::shared_ptr<TransitionAST> node);
	void visit(std::shared_ptr<InputTransition> node);
	void visit(std::shared_ptr<OutputTransition> node);
	void visit(std::shared_ptr<TauTransition> node);
	void visit(std::shared_ptr<PBranch> node);
	void visit(std::shared_ptr<Effect> node);
	void visit(std::shared_ptr<Assignment> node);
	void visit(std::shared_ptr<ClockReset> node);
	void visit(std::shared_ptr<Dist> node);
	void visit(std::shared_ptr<SingleParameterDist> node);
	void visit(std::shared_ptr<MultipleParameterDist> node);
	void visit(std::shared_ptr<Location> node);
	void visit(std::shared_ptr<ArrayPosition> node);
	void visit(std::shared_ptr<IConst> node);
	void visit(std::shared_ptr<BConst> node);
	void visit(std::shared_ptr<FConst> node);
	void visit(std::shared_ptr<LocExp> node);
	void visit(std::shared_ptr<OpExp> node);
	void visit(std::shared_ptr<BinOpExp> node);
	void visit(std::shared_ptr<UnOpExp> node);
	void visit(std::shared_ptr<Prop> node);
	void visit(std::shared_ptr<TransientProp> node);
	void visit(std::shared_ptr<RateProp> node);
	void visit(std::shared_ptr<TBoundSSProp> node);
	static string to_str(Type type);
    static string to_str(LabelType type);
    static string to_str(DistType type);
    static string to_str(PropType type);
    static string to_str(ExpOp type);
};

#endif
