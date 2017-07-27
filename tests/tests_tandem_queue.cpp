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


namespace  // // // // // // // // // // // // // // // // // // // // // // //
{

const string MODEL(tests::MODELS_DIR + "tandem_queue.sa");
auto& model(fig::ModelSuite::get_instance());
ModelPrinter printer(cerr, false);
size_t trPropId;
size_t ssPropId;

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

TEST_CASE("Tandem queue tests", "[tandem-queue]")
{

SECTION("Compile model file")
{
	REQUIRE(compile_model(MODEL));
	REQUIRE_FALSE(model.sealed());
	// XXX ModelSuite is a singleton: the compiled model is available
	//                                to all following SECTION blocks
	CHECK(model.num_modules() > 0ul);
}

SECTION("Add properties to verify")
{
	REQUIRE_FALSE(model.sealed());

	// Create an AST variable for the counter of the second queue
	auto q2 = make_shared<Location>("q2");
	q2->set_decl(ModuleScope::find_identifier_on(
		CompositeModuleScope::get_instance(), "q2"));
	REQUIRE(nullptr != q2->get_decl());
	assert(!q2->is_array_position());
	assert(!q2->get_decl()->is_array());

	// Rare event (RHS of transient prop): q2 == 8
	auto goal = make_shared<BinOpExp>(ExpOp::eq,
									  make_shared<LocExp>(q2),
									  make_shared<IConst>(8));
	REQUIRE(nullptr != goal);

	// Stop event (LHS of transient prop): q2 > 0
	auto noStop = make_shared<BinOpExp>(ExpOp::gt,
										make_shared<LocExp>(q2),
										make_shared<IConst>(0));
	REQUIRE(nullptr != noStop);

	// Steady-state property: S( rare )
	auto ssProp = make_shared<fig::PropertyRate>(goal);
	REQUIRE(nullptr != ssProp);
	ssPropId = model.add_property(ssProp);
	ssProp.reset();
	REQUIRE(nullptr != model.get_property(ssPropId));

	// Transient property: P( !stop U rare )
	auto trProp = make_shared<fig::PropertyTransient>(noStop, goal);
	REQUIRE(nullptr != trProp);
	trPropId = model.add_property(trProp);
	trProp.reset();
	REQUIRE(nullptr != model.get_property(trPropId));
}

SECTION("Seal model and check consistency")
{
	REQUIRE(seal_model());
	REQUIRE(model.num_modules() > 0ul);
	REQUIRE(model.num_properties() >= 2ul);
	REQUIRE(model.num_simulators() > 0ul);
	REQUIRE(model.num_importance_functions() > 0ul);
	REQUIRE(model.num_importance_strategies() > 0ul);
	REQUIRE(model.num_threshold_techniques() > 0ul);
	REQUIRE(model.num_RNGs() > 0ul);
}

SECTION("Estimate transient property using standard MC")
{
	const string nameEngine("nosplit");
	const string nameIFun("algebraic");
	const string nameThr("fix");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(nameIFun));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_splitting(1);
	model.build_importance_function_flat(nameIFun, trPropId, true);
	model.build_thresholds(nameThr, nameIFun);
	auto engine = model.prepare_simulation_engine(nameEngine, nameIFun);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 8);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(10);  // estimate for 10 seconds
	// Estimate
	model.estimate(trPropId, *engine, timeBound);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	const auto& ci = results.front();
	const double TR_PROB(5.6e-6);  // true transient probability of the RE
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.8));
	REQUIRE(ci.precision(.9) > 0.0);
	REQUIRE(ci.precision(.9) < TR_PROB*1.5);
}

	
}

} // namespace tests   // // // // // // // // // // // // // // // // // // //
