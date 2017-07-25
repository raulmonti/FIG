//==============================================================================
//
//  tests_tandem_queue.cpp
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


#include <tests_definitions.h>


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

TEST_CASE("Tandem queue tests", "[tandem-queue]")
{

const string MODEL(MODELS_DIR + "tandem_queue.sa");
auto& model(fig::ModelSuite::get_instance());

SECTION("Compile model file")
{
	REQUIRE(compile_model(MODEL));
	REQUIRE(model.sealed());
	// XXX ModelSuite is a singleton: the compiled model is available
	//                                to all following SECTION blocks
}

SECTION("Model compiled is consistent")
{
	REQUIRE(model.sealed());
	REQUIRE(model.num_modules() > 0ul);
	CHECK(model.num_properties() > 0ul);  // we need some property to verify!
	REQUIRE(model.num_simulators() > 0ul);
	REQUIRE(model.num_importance_functions() > 0ul);
	REQUIRE(model.num_importance_strategies() > 0ul);
	REQUIRE(model.num_threshold_techniques() > 0ul);
	REQUIRE(model.num_RNGs() > 0ul);
}

SECTION("Add properties to verify")
{
	auto U_LHS = make_shared<BinOpExp>(ExpOp::gt,
										make_shared<LocExp>("q2"),
										make_shared<IConst>(0));
	auto U_RHS = make_shared<BinOpExp>(ExpOp::eq,
										make_shared<LocExp>("q2"),
										make_shared<IConst>(8));
	/// @todo TODO continue here

	//model.add_property
}

SECTION("Change simulation environment")
{
	
}

	
}

} // namespace tests   // // // // // // // // // // // // // // // // // // //
