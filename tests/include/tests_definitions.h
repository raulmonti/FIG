//==============================================================================
//
//  tests_definitions.h
//	
//	Copyleft 2017-
//	Authors:
//  * Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
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


#ifndef TESTS_DEFINITIONS_H
#define TESTS_DEFINITIONS_H

// C++
#include <string>
#include <memory>
#include <chrono>
#include <iostream>
#include <type_traits>  // std::is_same<>...
// External code
#include <catch.hpp>
// FIG
#include <fig.h>


// Prolongue timeouts for DEBUG builds
#ifndef TIMEOUT_
#  ifndef NDEBUG
	 template<typename I> I TIMEOUT_(const I& x)
	 {
		static_assert(std::is_integral<I>::value, "ERROR: expected integral type");
		return 3*x;
	 }
#  else
#    define TIMEOUT_(x) (x)
#  endif
#endif

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::shared_ptr;
using std::make_shared;


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

/// Print build preamble in \p out stream
void preamble_testcase(std::ostream& out, const string& suffix = "");

/// Absolute path where (tests) model files reside
const string& models_dir();

/**
 * Default IOSA model compilation
 * @param modelFilePath Full path to the model file
 * @return Whether the model file could be successfully compiled
 * @note The model is compiled and built <b>but not sealed</b>
 * @see seal_current_model()
 * @warning Must be called from within a TEST_CASE
 */
bool compile_model(const string& modelFilePath);

/**
 * Seal model for simulation
 * @return Whether the model file could be successfully sealed
 * @note The model must have been built beforehand
 * @see build_model()
 * @warning Must be called from within a TEST_CASE
 */
bool seal_model();

} // namespace tests   // // // // // // // // // // // // // // // // // // //

#endif

