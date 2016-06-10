//==============================================================================
//
//  ImportanceFunctionConcreteSplit.h
//
//  Copyleft 2015-
//  Authors:
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


#ifndef IMPORTANCEFUNCTIONCONCRETESPLIT_H
#define IMPORTANCEFUNCTIONCONCRETESPLIT_H

// C++
#include <array>
#include <vector>
#include <string>
// FIG
#include <FigException.h>
#include <ImportanceFunctionConcrete.h>


namespace parser { class DNFclauses; }  // Fwd declaration

namespace fig
{

class ModuleInstance;
class ModuleNetwork;

/**
 * @brief ImportanceFunction for the concrete importance assessment
 *        of the ModuleInstance objects composing the ModuleNetwork.
 *
 *        Assesses the importance of the concrete state space of every
 *        individual module, viz. the "split" view of the user model.
 *        This requires access to all \ref ModuleInstance "system modules"
 *        with their \ref State "symbolic states", namely the arrays with
 *        the \ref Variable "variables" uniquely defined in each of them.
 *
 * @see ImportanceFunction
 * @see ImportanceFunctionConcrete
 * @see ImportanceFunctionConcreteCoupled
 */
class ImportanceFunctionConcreteSplit : public ImportanceFunctionConcrete
{
public:

	/// Codes for the composition strategy of the modules importance values
	enum CompositionType
	{
		SUMMATION = 0,  // m1 + m2 + ··· + mN
		PRODUCT,        // m1 * m2 * ··· * mN
		MAX,            // max(m1, m2, ..., mN)
		MIN,            // min(m1, m2, ..., mN)
		AD_HOC,         // user defined algebraic formula
		NUM_TYPES,  // must be defined before last
		INVALID     // must be defined last
	};

	/// Valid operands interpreted as a composition strategy
	static const std::array< std::string, 4 > compositionOperands;

private:

	/// All the \ref ModuleInstance "modules" in the network
	const std::vector< std::shared_ptr< ModuleInstance > >& modules_;

	/// Number of \ref ModuleInstance "modules" in the network
	const size_t numModules_;

	/// Whether each module is relevant for importance computation
	std::vector<bool> isRelevant_;

	/// Position, in a global system state, of the first variable of each module
	static std::vector< unsigned > globalVarsIPos;

	/// Temporal storage for the local importance
	/// computed for each ("split") ModuleInstance
	mutable ImportanceVec localValues_;

	/// Copy of the local states of the system \ref ModuleInstance "modules"
	mutable std::vector< State< STATE_INTERNAL_TYPE > > localStatesCopies_;

    /// Strategy to used for composing the importance values of the modules
    CompositionType compositionStrategy_;

    /// (Optional) User-defined minimal value of the composition function
    ImportanceValue userMinValue_;

    /// (Optional) User-defined maximal value of the composition function
    ImportanceValue userMaxValue_;

    /// Value of the neutral element for the composition strategy chosen
    ImportanceValue neutralElement_;

    /// Property to check, parsed as a DNF formula
    parser::DNFclauses propertyClauses;

    /// @copydoc ImportanceFunction::concrete_simulation()
    bool concreteSimulation_;

public:  // Ctor/Dtor

	/// @brief Data ctor
	/// @param model System model, whose current state is taken as initial
	ImportanceFunctionConcreteSplit(const ModuleNetwork& model);

	/// Dtor
	virtual ~ImportanceFunctionConcreteSplit();

	/// Avoid accidental copies
	ImportanceFunctionConcreteSplit(const ImportanceFunctionConcreteSplit&) = delete;

	/// Avoid accidental copies
	ImportanceFunctionConcreteSplit&
	operator=(const ImportanceFunctionConcreteSplit&) = delete;

public:  // Accessors

	inline bool concrete_simulation() const noexcept override final { return concreteSimulation_; }

	/// @copydoc ImportanceFunctionConcrete::info_of()
	/// @note <b>Complexity:</b> <i>O(size(state))</i> +
	///                          <i>O(mu::Parser::Eval(localValues_))</i>
	ImportanceValue info_of(const StateInstance& state) const override;

	/// @copydoc ImportanceFunctionConcrete::importance_of()
	/// @note <b>Complexity:</b> <i>O(size(state))</i> +
	///                          <i>O(mu::Parser::Eval(localValues_))</i>
	ImportanceValue importance_of(const StateInstance& state) const override;

	void print_out(std::ostream& out, State<STATE_INTERNAL_TYPE> s) const override;

public:  // Utils

	/**
	 * @brief Set the function to use for composing the stored importance
	 *        values of the \ref ModuleInstance "modules".
	 *
	 *        Either an operand (any from compositionOperands, e.g. "max", "+")
	 *        or a fully defined algebraic expression with explicit module names
	 *        (e.g. "5*Queue1+Queue2") can be specified as composition function.
	 *        The operands are associative and will be applied to all modules.
	 *
	 * @param compFunExpr Algebraic expression to use as composition function.
	 * @param nullVal Neutral element of the algebraic expression, needed only
	 *                for fully defined functions (i.e. not for operands)
	 * @param minVal  Optional user-defined min value of the algebraic expression
	 * @param maxVal  Optional user-defined max value of the algebraic expression
	 *
	 * @throw FigException if invalid or badly formatted function expression
	 */
	void set_composition_fun(std::string compFunExpr,
							 const ImportanceValue& nullVal = 0u,
							 const ImportanceValue& minVal  = 0u,
							 const ImportanceValue& maxVal  = 0u);

	void assess_importance(const Property& prop,
						   const std::string& strategy = "flat",
						   const PPSpec& postProc = std::make_pair("",.0)) override;

private:

	/// ImportanceFunctionConcreteSplit for 'adhoc' assessment strategy is
	/// currently unavailable
	/// @deprecated The idea is too complicated and little rewarding:
	///   it'd require the user's algebraic formula for importance computation
	///   *plus* another algebraic formula to compose the modules importance.
	///   Symbolic storage (i.e. ImportanceFunctionAlgebraic) is all the
	///   'adhoc' importance assessment strategy needs. Go bother them.
	inline void assess_importance(const Property&,
								  const std::string&,
								  const std::vector<std::string>&) override
		{ throw_FigException("unavailable member function"); }

private:  // Class utils

	/**
	 * Compose a composition function (pun intended) combining all modules'
	 * importance using given (valid) algebraic operand
	 * @param modulesNames Names of all \ref ModuleInstance "modules"
	 * @param compOperand Algebraic operand to use
	 * @throw FigException if 'compOperand' is not in compositionOperands
	 * @note Updates the internal fields 'compositionStrategy_' and 'neutralElement_'
	 * @see compositionOperands
	 */
	std::string compose_comp_function(
			const std::vector<std::string>& modulesNames,
			const std::string& compOperand);
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONCONCRETESPLIT_H

