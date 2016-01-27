//==============================================================================
//
//  ThresholdsBuilderAMS.h
//
//  Copyleft 2016-
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

#ifndef THRESHOLDSBUILDERAMS_H
#define THRESHOLDSBUILDERAMS_H

// C++
#include <tuple>
#include <type_traits>  // std::is_constructible<>
// FIG
#include <ThresholdsBuilder.h>
#include <FigException.h>


namespace fig
{

using get = std::get;

/**
 * @brief ThresholdsBuilder implementing Adaptive Multilevel Splitting (AMS)
 *
 *        AMS is a thresholds building technique which takes its name from
 *        an article published by Cerou and Guyader in 2007.
 *        Given a state space and an importance function on it, AMS aims to
 *        locate the thresholds so that all the probabilities 'P_i' are roughly
 *        the same. Here 'P_i' is defined as the conditional probability of a
 *        simulation run going up the i-th importance threshold, given that it
 *        started at the (i-1)-th importance threshold.
 */
class ThresholdsBuilderAMS : public ThresholdsBuilder
{
	/// Number of simulations to launch for threshold construction
	unsigned n_;

	/// Number of surviving simulations to consider
	unsigned k_;

	/// Number of thresholds built on last invocation
	int numThresholds_;

	/// Lowest level containing a rare state, from last invocation
	int minRareLvl_;

public:

	template< typename T_ >
	virtual void
	tune(const State<STATE_INTERNAL_TYPE>& state,
		 const T_* initData = nullptr)
		{
			if (!std::is_convertible< std::tuple<unsigned,unsigned,unsigned>, T>::value)
				throw_FigException("ERROR: type mismatch. Invalid tune data "
								   "for ThresholdsBuilderAMS");
			auto data = *reinterpret_cast< const std::tuple<unsigned,
															unsigned,
															unsigned>* >(initData);
			tune(state.concrete_size(), get<0>(data), get<1>(data), get<2>(data));
		}

	/**
	 * @brief Choose values for n_ and k_ depending on the nature of the Module
	 *        (states and transitions space size) and the simulation.
	 * @param numStates      Size of the concrete state space in the Module
	 * @param numTrans       Number of (symbolic) transitions in the Module
	 * @param maxImportance  Maximum ImportanceValue computed
	 * @param splitsPerThr   Number of splits upon a threshold level-up
	 */
	void tune(const size_t&   numStates,
			  const unsigned& numTrans,
			  const unsigned& maxImportance,
			  const unsigned& splitsPerThr);

	virtual void
	build_thresholds_concrete(const AdjacencyList& edges,
							  std::vector< ImportanceValue >& impVec) const;
};

} // namespace fig

#endif // THRESHOLDSBUILDERAMS_H
