//==============================================================================
//
//  ModelAST.cpp
//
//  Copyleft 2016-
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

// C++
#include <cstdlib>
#include <cstring>
// fig
#include <ModelAST.h>
#include <ModelParser.hpp>
#include <ErrorMessage.h>
#include <FigLog.h>
#include <FigException.h>

using std::shared_ptr;
using std::static_pointer_cast;
using fig::figTechLog;

void ModelAST::accept(Visitor &visit) {
    visit.visit(shared_from_this());
}

shared_ptr<ModelAST> ModelAST::from_files(const std::string& model_file,
                                          const std::string& prop_file) {
	shared_ptr<ModelAST> result = nullptr;
	ModelParserGen::ModelParser parser {&result};
	int res(1);
	// Process model file
	FILE* file = std::fopen(model_file.c_str(), "r");
	if (nullptr == file && !model_file.empty()) {
		figTechLog << "Model file \"" << model_file << "\" does not exists!\n";
		res = 1;
		goto exit_point;
	} else if (model_file.empty()) {
		goto exit_point;
	}
	scan_begin(file);
	res = parser.parse();
	scan_end();
	if (0 != res || nullptr == result) {
		figTechLog << "Errors found while parsing \"" << model_file << "\"\n";
		goto exit_point;
	}
	// Process properties file, if any
	if (!prop_file.empty()) {
		file = fopen(prop_file.c_str(), "r");
		if (nullptr == file) {
			figTechLog << "Properties file \"" << prop_file
							<< "\" does not exists!\n";
			res = 1;
			goto exit_point;
		}
		scan_begin(file);
		res = parser.parse();
		scan_end();
		if (0 != res) {
			figTechLog << "Errors found while parsing \"" << prop_file << "\"\n";
			goto exit_point;
		}
	}
	exit_point:
	    if (nullptr != file && 0 <= fileno(file))
			std::fclose(file);
		return (res == 0 ? result : nullptr);
}

void ModelAST::on_scanner_error(const string &msg) {
	figTechLog << "Syntax error: " << msg << std::endl;
}

std::set<string> Model::get_labels() const {
	std::set<string> result;
	for (const auto& m_ptr: modules) {
		for (const auto& t_ptr: m_ptr->get_transitions()) {
			result.emplace(t_ptr->get_label());
		}
	}
	return result;
}

bool Model::has_module(const string &id) {
    // Is there a model with the given name?
    auto same_name = [id] (shared_ptr<ModuleAST> module) {
        return (module->get_name() == id);
    };
    return (std::find_if(modules.begin(),
                         modules.end(), same_name) != modules.end());
}

void Model::accept(Visitor& visit) {
    //my_type == Model
    using my_type = std::remove_pointer<decltype(this)>::type;
    const auto &_this = static_pointer_cast<my_type>(shared_from_this());
    visit.visit(_this);
}

void ModuleAST::accept(Visitor& visit) {
    using my_type = std::remove_pointer<decltype(this)>::type;
    visit.visit(static_pointer_cast<my_type>(shared_from_this()));
}

bool ModuleAST::has_committed_actions() const {
    for (const shared_ptr<TransitionAST>& tr : this->get_transitions()) {
        if (tr->get_label_type() == LabelType::in_committed) {
            return (true);
        }
        if (tr->get_label_type() == LabelType::out_committed) {
            return (true);
        }
    }
    return (false);
}

bool ModuleAST::has_arrays() const {
    for (const shared_ptr<Decl> &decl : get_local_decls()) {
        if (decl->is_array()) {
            return (true);
        }
    }
    return (false);
}


void Decl::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Decl>(shared_from_this()));
}

void InitializedDecl::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<InitializedDecl>(shared_from_this()));
}

void RangedDecl::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<RangedDecl>(shared_from_this()));
}

void ClockDecl::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<ClockDecl>(shared_from_this()));
}

void ArrayDecl::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<ArrayDecl>(shared_from_this()));
}

void InitializedArray::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<InitializedArray>(shared_from_this()));
}

void MultipleInitializedArray::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<MultipleInitializedArray>(shared_from_this()));
}

void RangedInitializedArray::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<RangedInitializedArray>(shared_from_this()));
}

void RangedMultipleInitializedArray::accept(Visitor& visit) {
	visit.visit(static_pointer_cast<RangedMultipleInitializedArray>(shared_from_this()));
}

const shared_vector<Assignment>&
TransitionAST::get_assignments()
{
	if (allAssignments.empty() && !pBranches.empty())
		for (const auto& pbranch: pBranches)
			for (auto ass: pbranch->get_assignments())
				allAssignments.emplace_back(ass);
	return allAssignments;
}

const shared_vector<ClockReset>&
TransitionAST::get_clock_resets()
{
	if (allClockResets.empty() && !pBranches.empty())
		for (const auto& pbranch: pBranches)
			for (auto creset: pbranch->get_clock_resets())
				allClockResets.emplace_back(creset);
	return allClockResets;
}

PBranch::PBranch(std::shared_ptr< Exp > probability,
                 shared_vector< Effect > effects) :
    pWeight(probability)
{
	for (auto effect: effects) {
		if (effect->is_assignment()) {
			assignments.push_back(static_pointer_cast<Assignment>(effect));
		} else if (effect->is_clock_reset()) {
			clockResets.push_back(static_pointer_cast<ClockReset>(effect));
		} else {
			throw_FigException("unknown RHS in probabilistic branch");
		}
	}
}

void TransitionAST::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<TransitionAST>(shared_from_this()));
}

void OutputTransition::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<OutputTransition>(shared_from_this()));
}

void TauTransition::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<TauTransition>(shared_from_this()));
}

void InputTransition::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<InputTransition>(shared_from_this()));
}

void InputCommittedTransition::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<InputCommittedTransition>(shared_from_this()));
}

void OutputCommittedTransition::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<OutputCommittedTransition>(shared_from_this()));
}

void PBranch::accept(Visitor& visit) {
	visit.visit(static_pointer_cast<PBranch>(shared_from_this()));
}

void Effect::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Effect>(shared_from_this()));
}

void Assignment::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Assignment>(shared_from_this()));
}

void ClockReset::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<ClockReset>(shared_from_this()));
}

void Dist::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Dist>(shared_from_this()));
}

void SingleParameterDist::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<SingleParameterDist>(shared_from_this()));
}

void MultipleParameterDist::accept(Visitor &visit) {
    visit.visit(static_pointer_cast<MultipleParameterDist>(shared_from_this()));
}

void Location::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Location>(shared_from_this()));
}

void ArrayPosition::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<ArrayPosition>(shared_from_this()));
}

std::string ArrayPosition::to_string() const noexcept {
    return (this->get_identifier() + "[" + index->to_string() + "]");
}

void Prop::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Prop>(shared_from_this()));
}

void TransientProp::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<TransientProp>(shared_from_this()));
}

void RateProp::accept(Visitor& visit) {
	visit.visit(static_pointer_cast<RateProp>(shared_from_this()));
}

void TBoundSSProp::accept(Visitor& visit) {
	visit.visit(static_pointer_cast<TBoundSSProp>(shared_from_this()));
}

//Exp
void Exp::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<Exp>(shared_from_this()));
}

//LocExp
void LocExp::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<LocExp>(shared_from_this()));
}

//IConst
void IConst::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<IConst>(shared_from_this()));
}

//BConst
void BConst::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<BConst>(shared_from_this()));
}

//OpExp
void OpExp::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<OpExp>(shared_from_this()));
}

void BinOpExp::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<BinOpExp>(shared_from_this()));
}

void UnOpExp::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<UnOpExp>(shared_from_this()));
}

//FConst
void FConst::accept(Visitor& visit) {
    visit.visit(static_pointer_cast<FConst>(shared_from_this()));
}

//Default Visitor does nothing on his visitation ;)
Visitor::Visitor() :
    message(make_shared<ErrorMessage>())
{ /* Not much to do around here */ }

void Visitor::ignore_errors() {
    message->ignore_errors();
}

void Visitor::put_error(const string &msg) {
    message->put_error(msg);
}

void Visitor::put_warning(const string &msg) {
    message->put_warning(msg);
}

bool Visitor::has_errors() {
    return (message->has_errors());
}

bool Visitor::has_warnings() {
    return (message->has_warnings());
}

string Visitor::get_messages() {
    return (message->get_msg());
}

void Visitor::visit(shared_ptr<ModelAST>) { /* NOP */ }

//Model
void Visitor::visit(shared_ptr<Model> node) {
    visit(std::static_pointer_cast<ModelAST>(node));
}

// Module
void Visitor::visit(shared_ptr<ModuleAST> node) {
    visit(std::static_pointer_cast<ModelAST>(node));
}

// Properties
void Visitor::visit(shared_ptr<Prop> node) {
    visit(std::static_pointer_cast<ModelAST>(node));
}
void Visitor::visit(shared_ptr<TransientProp> node) {
    visit(std::static_pointer_cast<Prop>(node));
}
void Visitor::visit(shared_ptr<RateProp> node) {
	visit(std::static_pointer_cast<Prop>(node));
}
void Visitor::visit(shared_ptr<TBoundSSProp> node) {
    visit(std::static_pointer_cast<Prop>(node));
}

// Declarations
void Visitor::visit(shared_ptr<Decl> node) {
    visit(std::static_pointer_cast<ModelAST>(node));
}
void Visitor::visit(shared_ptr<InitializedDecl> node) {
    visit(std::static_pointer_cast<Decl>(node));
}
void Visitor::visit(shared_ptr<RangedDecl> node) {
    visit(std::static_pointer_cast<Decl>(node));
}
void Visitor::visit(shared_ptr<ClockDecl> node) {
    visit(std::static_pointer_cast<Decl>(node));
}
void Visitor::visit(shared_ptr<ArrayDecl> node) {
    visit(std::static_pointer_cast<Decl>(node));
}
void Visitor::visit(shared_ptr<InitializedArray> node) {
    visit(std::static_pointer_cast<ArrayDecl>(node));
}

void Visitor::visit(shared_ptr<MultipleInitializedArray> node) {
    visit(std::static_pointer_cast<ArrayDecl>(node));
}

void Visitor::visit(shared_ptr<RangedInitializedArray> node) {
    visit(std::static_pointer_cast<ArrayDecl>(node));
}

void Visitor::visit(shared_ptr<RangedMultipleInitializedArray> node)  {
    visit(std::static_pointer_cast<ArrayDecl>(node));
}

//Transitions
void Visitor::visit(shared_ptr<TransitionAST> node) {
    visit(std::static_pointer_cast<ModelAST>(node));
}
void Visitor::visit(shared_ptr<OutputTransition> node) {
    visit(std::static_pointer_cast<TransitionAST>(node));
}
void Visitor::visit(shared_ptr<TauTransition> node) {
    visit(std::static_pointer_cast<OutputTransition>(node));
}
void Visitor::visit(shared_ptr<WildcardInputTransition> node) {
    visit(std::static_pointer_cast<TransitionAST>(node));
}
void Visitor::visit(shared_ptr<InputTransition> node) {
    visit(std::static_pointer_cast<TransitionAST>(node));
}
void Visitor::visit(shared_ptr<InputCommittedTransition> node) {
    visit(std::static_pointer_cast<TransitionAST>(node));
}
void Visitor::visit(shared_ptr<OutputCommittedTransition> node) {
    visit(std::static_pointer_cast<TransitionAST>(node));
}

//Effects
void Visitor::visit(shared_ptr<PBranch> node) {
	visit(std::static_pointer_cast<ModelAST>(node));
}
void Visitor::visit(shared_ptr<Effect> node) {
    visit(std::static_pointer_cast<ModelAST>(node));
}
void Visitor::visit(shared_ptr<Assignment> node) {
    visit(std::static_pointer_cast<Effect>(node));
}
void Visitor::visit(shared_ptr<ClockReset> node) {
    visit(std::static_pointer_cast<Effect>(node));
}

//Distributions
void Visitor::visit(shared_ptr<Dist> node) {
    visit(std::static_pointer_cast<ModelAST>(node));
}
void Visitor::visit(shared_ptr<SingleParameterDist> node) {
    visit(std::static_pointer_cast<Dist>(node));
}
void Visitor::visit(shared_ptr<MultipleParameterDist> node) {
    visit(std::static_pointer_cast<Dist>(node));
}

//Locations
void Visitor::visit(shared_ptr<Location> node) {
    visit(std::static_pointer_cast<ModelAST>(node));
}
void Visitor::visit(shared_ptr<ArrayPosition> node) {
    visit(std::static_pointer_cast<Location>(node));
}

//Expressions
void Visitor::visit(shared_ptr<Exp> node)  {
    visit(std::static_pointer_cast<ModelAST>(node));
}
void Visitor::visit(shared_ptr<IConst> node) {
    visit(std::static_pointer_cast<Exp>(node));
}
void Visitor::visit(shared_ptr<BConst> node) {
    visit(std::static_pointer_cast<Exp>(node));
}
void Visitor::visit(shared_ptr<FConst> node) {
    visit(std::static_pointer_cast<Exp>(node));
}
void Visitor::visit(shared_ptr<LocExp> node) {
    visit(std::static_pointer_cast<Exp>(node));
}
void Visitor::visit(shared_ptr<OpExp> node)  {
    visit(std::static_pointer_cast<Exp>(node));
}
void Visitor::visit(shared_ptr<BinOpExp> node) {
    visit(std::static_pointer_cast<OpExp>(node));
}
void Visitor::visit(shared_ptr<UnOpExp> node)  {
    visit(std::static_pointer_cast<OpExp>(node));
}
