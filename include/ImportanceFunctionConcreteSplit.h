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
#include <ModuleInstance.h>
#include <ModuleNetwork.h>


namespace fig
{

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

	/// Valid operands for combining the "split modules" importance values
	static const std::array< std::string, 4 > mergeOperands;

	/// Codes for the combination/merge strategy
	/// of the "split modules" importance values
	enum MergeType
	{
		NONE,
		SUMMATION,  // m1 + m2 + ··· + mN
		PRODUCT,    // m1 * m2 * ··· * mN
		MAX,        // max(m1, m2, ..., mN)
		MIN,        // min(m1, m2, ..., mN)
		AD_HOC      // user defined algebraic formula
	};

private:

	/// All the \ref ModuleInstance "modules" in the network
	const std::vector< std::shared_ptr< ModuleInstance > >& modules_;

	/// Number of \ref ModuleInstance "modules" in the network
	const size_t numModules_;

	/// Position, in a global system state, of the first variable of each module
	static std::vector< unsigned > globalVarsIPos;

	/// Temporal storage for the local importance
	/// computed for each ("split") ModuleInstance
	mutable ImportanceVec localValues_;

	/// Copy of the local states of the system \ref ModuleInstance "modules"
	mutable std::vector< State< STATE_INTERNAL_TYPE > > localStatesCopies_;

    /// Strategy used to merge the "split modules" importance values
    mutable MergeType mergeStrategy_;

    /// Translator from a global state ImportanceValue to its threshold level
    std::vector< ImportanceValue > importance2threshold_;

public:  // Ctor/Dtor

	/// @brief Data ctor
	/// @param model System model, whose current state is taken as initial
	ImportanceFunctionConcreteSplit(const ModuleNetwork& model);

	/// Dtor
	virtual ~ImportanceFunctionConcreteSplit();

public:  // Accessors

	/// @copydoc ImportanceFunctionConcrete::info_of()
	/// @note <b>Complexity:</b> <i>O(size(state))</i> +
	///                          <i>O(mu::Parser::Eval(localValues_))</i>
	ImportanceValue info_of(const StateInstance& state) const override;

	/// @copydoc ImportanceFunctionConcrete::importance_of()
	/// @note <b>Complexity:</b> <i>O(size(state))</i> +
	///                          <i>O(mu::Parser::Eval(localValues_))</i>
	ImportanceValue importance_of(const StateInstance& state) const override;

	/// @copydoc ImportanceFunction::level_of(const StateInstance&)
	/// @note Attempted inline in a desperate need for speed
	/// @note <b>Complexity:</b> same as ImportanceFunctionConcreteSplit::importance_of()
	inline ImportanceValue level_of(const StateInstance& state) const override
		{
#		ifndef NDEBUG
			if (!ready())
				throw_FigException("importance function \"" + name() + "\" "
								   "isn't ready for simulations.");
#		endif
			return importance2threshold_[importance_of(state)];
		}

	/// @copydoc ImportanceFunction::level_of(const ImportanceValue&)
	/// @note Attempted inline in a desperate need for speed
	/// @note <b>Complexity:</b> <i>O(1)</i>
	inline ImportanceValue level_of(const ImportanceValue& val) const override
		{
#		ifndef NDEBUG
			if (!ready())
				throw_FigException("importance function \"" + name() + "\" "
								   "isn't ready for simulations.");
#		endif
			assert(val < importance2threshold_.size());
			return importance2threshold_[val];
		}

	void print_out(std::ostream& out, State<STATE_INTERNAL_TYPE> s) const override;

public:  // Utils

	/**
	 * @brief Set the function to use for combining the stored importance
	 *        values of the \ref ModuleInstance "split modules".
	 * @param mergeFunExpr Algebraic expression to use as merging function.
	 *                     This can either be a single operand which would be
	 *                     applied to all modules (e.g. "max", "+"), or a fully
	 *                     defined function with explicit module names
	 *                     (e.g. "5*Queue1+Queue2")
	 * @throw FigException if invalid or badly formatted function expression
	 */
	void set_merge_fun(std::string mergeFunExpr);

	void assess_importance(const Property& prop,
						   const std::string& strategy = "flat") override;

	void assess_importance(const Property& prop,
						   const std::string& formulaExprStr,
						   const std::vector<std::string>& varnames) override;

	void build_thresholds(ThresholdsBuilder& tb, const unsigned& spt) override;

	void build_thresholds_adaptively(ThresholdsBuilderAdaptive& atb,
									 const unsigned& spt,
									 const float& p,
									 const unsigned& n) override;

	void clear() noexcept override;

private:  // Class utils

	/**
	 * Compose a merge function which combines all modules' importance
	 * using given (valid) algebraic operand
	 * @param modulesNames Names of all \ref ModuleInstance "modules"
	 * @param mergeOperand Algebraic operand to use
	 * @throw FigException if 'mergeOperand' is not in mergeOperands
	 * @see mergeOperands
	 */
	std::string compose_merge_function(
			const std::vector<std::string>& modulesNames,
			const std::string& mergeOperand) const;

	/// Post-processing once the thresholds have been chosen
	/// @param tbName  Name of the ThresholdsBuilder used
	/// @see build_thresholds()
	/// @see build_thresholds_adaptively()
	void post_process_thresholds(const std::string& tbName);
};

} // namespace fig

#endif // IMPORTANCEFUNCTIONCONCRETESPLIT_H

