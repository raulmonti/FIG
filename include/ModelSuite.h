//==============================================================================
//
//  ModelSuite.h
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

#ifndef MODELSUITE_H
#define MODELSUITE_H

// C++
#include <mutex>  // std::call_once(), std::once_flag
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
// FIG
#include <ModuleNetwork.h>
#include <ImportanceFunction.h>


namespace fig
{

class ModelSuite
{
	/// User's system model
	static ModuleNetwork model;
	
//	/// Properties to estimate
//	static std::vector<Property> properties;
	
//	/// Confidence criteria or time budgets bounding simulations
//	static StoppingCondition goal;
	
	/// Importance functions available
	static std::unordered_map< std::string, ImportanceFunction* > iFuns;
	
//	/// Simulation engines available
//	static std::unordered_map< std::string, SimulationEngine& >   simulators;

	/// Single existent instance of the class (singleton design pattern)
	static std::unique_ptr< ModelSuite > instance_;

	/// Single instance thread safety
	static std::once_flag singleInstance_;

	/// Private ctors (singleton design pattern)
	ModelSuite() {}
	ModelSuite(ModelSuite&& that)                 = delete;
	ModelSuite& operator=(const ModelSuite& that) = delete;

public:  // Access to the ModelSuite instance

	/// Global access point to the unique instance of this pool
	static inline ModelSuite& get_instance()
		{
			std::call_once(singleInstance_,
						   [] () { instance_.reset(new ModelSuite); });
			return *instance_;
		}

	/// Allow syntax "auto varname = fig::ModelSuite::get_instance();"
	inline ModelSuite(const ModelSuite& that) {}

	~ModelSuite();

public:  // Stubs for ModuleNetwork

	/// @copydoc ModuleNetwork::add_module(std::shared_ptr<ModuleInstance>&)
	void add_module(std::shared_ptr< ModuleInstance >& module);

	/// @todo TODO copy all relevant public functions from ModuleNetwork

public:  // Current simulation configuration

	/**
	 * @brief Register importance function to use in the following estimations
	 * @details Grants access to importance info needed by some classes
	 *          during simulations.
	 * @param ifun Currently built importance function (null, auto, ad hoc...)
	 * @see ModuleNetwork::inspect()
	 */
	void set_current_ifun(const ImportanceFunction& ifun);

//	/**
//	 * @brief Register simulation engine to use in the following estimations
//	 * @param engine Simulation engine (nosplit, restart...)
//	 * @note  This grants access to information for those classes
//	 *        which need it during simulations
//	 *
//	 * Is this needed?
//	 *
//	 */
//	void set_current_engine(const SimulationEngine& engine);
};

} // namespace fig

#endif // MODELSUITE_H
