//==============================================================================
//
//  tests_0_resampling.cpp
//	
//	Copyleft 2021-
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

// Model files (full path to)
static const string M1 = "uniform";
static const string M2 = "Weibull";
static const string M3 = "tiny_RFT";
//static const string M4 = "queue_w_breakdowns";
static const string MODEL_1(tests::models_dir() + "resampling_" + M1 + ".sa");
static const string MODEL_2(tests::models_dir() + "resampling_" + M2 + ".sa");
static const string MODEL_3(tests::models_dir() + "resampling_" + M3 + ".sa");

// TAD which will contain the compiled model
fig::ModelSuite& model(fig::ModelSuite::get_instance());

// Expected results for property queries
// M1: Time-bounded steady-state: S [ 60:6060 ]( q >= 19)
constexpr double M1_PROB = 4.62e-8;
constexpr ulong M1_SECONDS = 30ul;
// M2: Time-bounded steady-state: S [ 60:6060 ]( q >= 19)
constexpr double M2_PROB = 3.89e-10;
constexpr double M2_PROB_EXTRA = 1.58e-9;
constexpr ulong M2_SECONDS = 45ul;
// M3: Transient: P ( ReliabilityTimeOut<2 U count_5==3 )
constexpr double M3_PROB = 2.65e-4;
constexpr ulong M3_SECONDS = 120ul;
// Property query idx (internal to TAD)
int propId(-1);
int propIdExtra(-1);

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

TEST_CASE("Resampling of clock values upon Traial copy", "[resampling]")
{

//  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //

SECTION("Compile model 1: resampling " + M1)
{
	preamble_testcase(fig::figTechLog, "resampling-" + M1);

    // If this is not the first test then we need to clean
	// the ModelSuite singleton before loading the new model
	if (model.sealed())
		model.clear();
	REQUIRE_FALSE(model.sealed());
	REQUIRE(compile_model(MODEL_1));
	// XXX ModelSuite is a singleton: the compiled model is available
	//                                to all following SECTION blocks
	REQUIRE_FALSE(model.sealed());
	CHECK(model.num_modules() > 0ul);

	// Register one steady-state property
	propId = -1;
	REQUIRE(model.num_properties() >= 1ul);
	for (size_t i = 0ul ; i < model.num_properties() ; i++) {
		auto prop = model.get_property(i);
		if (prop->type == fig::PropertyType::TBOUNDED_SS) {
			propId = i;
			break;
		}
	}
	REQUIRE(0 <= propId);
	REQUIRE(nullptr != model.get_property(propId));
}

SECTION("Seal model and check consistency")
{
	REQUIRE(seal_model());
	REQUIRE(model.num_modules() > 0ul);
	REQUIRE(model.num_properties() >= 1ul);
	REQUIRE(model.num_simulators() > 0ul);
	REQUIRE(model.num_importance_functions() > 0ul);
	REQUIRE(model.num_importance_strategies() > 0ul);
	REQUIRE(model.num_threshold_techniques() > 0ul);
	REQUIRE(model.num_RNGs() > 0ul);
}

SECTION("M1 without resampling (steady-state, RESTART, monolithic, ad hoc thresholds)")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_coupled", "auto");
	const string thrAdHoc("9:3,10:3,11:2,12:3,13:3,14:3,15:2,16:3,17:3,18:3");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(false);
	model.build_importance_function_auto(ifunSpec, propId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, propId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 911);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(M1_SECONDS));
	// Estimate
	model.estimate(propId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(M1_PROB).epsilon(M1_PROB*.2));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(M1_PROB*.3).epsilon(M1_PROB*.2));
}

SECTION("M1 with resampling (steady-state, RESTART, monolithic, ad hoc thresholds)")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_coupled", "auto");
	const string thrAdHoc("9:3,10:3,11:2,12:3,13:3,14:3,15:2,16:3,17:3,18:3");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(true);
	model.build_importance_function_auto(ifunSpec, propId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, propId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 911);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(M1_SECONDS));
	// Estimate
	model.estimate(propId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(M1_PROB).epsilon(M1_PROB*.2));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(M1_PROB*.3).epsilon(M1_PROB*.2));
}

//  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //

SECTION("Compile model 2: resampling " + M2)
{
	preamble_testcase(fig::figTechLog, "resampling-" + M2);

	// If this is not the first test then we need to clean
	// the ModelSuite singleton before loading the new model
	if (model.sealed())
		model.clear();
	REQUIRE_FALSE(model.sealed());
	REQUIRE(compile_model(MODEL_2));
	// XXX ModelSuite is a singleton: the compiled model is available
	//                                to all following SECTION blocks
	REQUIRE_FALSE(model.sealed());
	CHECK(model.num_modules() > 0ul);

	// Register one transient and one steady-state property
	propId = -1;
	propIdExtra = -1;
	REQUIRE(model.num_properties() >= 1ul);
	for (size_t i = 0ul ; i < model.num_properties() ; i++) {
		auto prop = model.get_property(i);
		if (prop->type == fig::PropertyType::TBOUNDED_SS) {
			propId = i;
		} else if (prop->type == fig::PropertyType::TRANSIENT) {
			propIdExtra = i;
		}
	}
	REQUIRE(0 <= propId);
	REQUIRE(0 <= propIdExtra);
	REQUIRE(nullptr != model.get_property(propId));
	REQUIRE(nullptr != model.get_property(propIdExtra));
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

SECTION("M2 without resampling (steady-state, RESTART2, monolithic, ad hoc thresholds)")
{
	const string nameEngine("restart2");
	const fig::ImpFunSpec ifunSpec("concrete_coupled", "auto");
	const string thrAdHoc("6:3,7:2,8:4,9:3,10:3,11:3,12:3,13:3,14:3");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(false, nameEngine);
	model.build_importance_function_auto(ifunSpec, propId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, propId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, std::mt19937_64::default_seed);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(M2_SECONDS));
	// Estimate
	model.estimate(propId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(M2_PROB).epsilon(M2_PROB*.2));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(M2_PROB*.5).epsilon(M2_PROB*.2));
}

SECTION("M2 with resampling (steady-state, RESTART2, monolithic, ad hoc thresholds)")
{
	const string nameEngine("restart2");
	const fig::ImpFunSpec ifunSpec("concrete_coupled", "auto");
	const string thrAdHoc("6:3,7:2,8:4,9:3,10:3,11:3,12:3,13:3,14:3");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(true, nameEngine);
	model.build_importance_function_auto(ifunSpec, propId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, propId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, std::mt19937_64::default_seed);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(M2_SECONDS));
	// Estimate
	model.estimate(propId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(M2_PROB).epsilon(M2_PROB*.2));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(M2_PROB*.5).epsilon(M2_PROB*.2));
}

SECTION("M2 without resampling (transient, Fixed Effort, monolithic, ad hoc thresholds)")
{
	const string nameEngine("sfe");
	const fig::ImpFunSpec ifunSpec("concrete_coupled", "auto");
	const string thrAdHoc("1:3,2:3,3:3,4:3,5:4,6:3,7:3,8:3,9:4,10:3,11:3,12:3,13:2,14:4");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(false, nameEngine);
	model.build_importance_function_auto(ifunSpec, propIdExtra, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, propIdExtra);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0XCAFEF00DD15EA5E5ULL);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(M2_SECONDS));
	// Estimate
	model.estimate(propIdExtra, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(M2_PROB_EXTRA).epsilon(M2_PROB_EXTRA*.2));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(M2_PROB_EXTRA*.5).epsilon(M2_PROB_EXTRA*.2));
}

SECTION("M2 with resampling (transient, Fixed Effort, monolithic, ad hoc thresholds)")
{
	const string nameEngine("sfe");
	const fig::ImpFunSpec ifunSpec("concrete_coupled", "auto");
	const string thrAdHoc("1:3,2:3,3:3,4:3,5:4,6:3,7:3,8:3,9:4,10:3,11:3,12:3,13:2,14:4");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(true, nameEngine);
	model.build_importance_function_auto(ifunSpec, propIdExtra, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, propIdExtra);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0XCAFEF00DD15EA5E5ULL);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(M2_SECONDS));
	// Estimate
	model.estimate(propIdExtra, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(M2_PROB_EXTRA).epsilon(M2_PROB_EXTRA*.2));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(M2_PROB_EXTRA*.5).epsilon(M2_PROB_EXTRA*.2));
}

//  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //  //

SECTION("Compile model 3: resampling " + M3)
{
	preamble_testcase(fig::figTechLog, "resampling-" + M3);

	// If this is not the first test then we need to clean
	// the ModelSuite singleton before loading the new model
	if (model.sealed())
		model.clear();
	REQUIRE_FALSE(model.sealed());
	REQUIRE(compile_model(MODEL_3));
	// XXX ModelSuite is a singleton: the compiled model is available
	//                                to all following SECTION blocks
	REQUIRE_FALSE(model.sealed());
	CHECK(model.num_modules() > 0ul);

	// Register one transient property
	propId = -1;
	REQUIRE(model.num_properties() >= 1ul);
	for (size_t i = 0ul ; i < model.num_properties() ; i++) {
		auto prop = model.get_property(i);
		if (prop->type == fig::PropertyType::TRANSIENT) {
			propId = i;
			break;
		}
	}
	REQUIRE(0 <= propId);
	REQUIRE(nullptr != model.get_property(propId));
}

SECTION("Seal model and check consistency")
{
	REQUIRE(seal_model());
	REQUIRE(model.num_modules() > 0ul);
	REQUIRE(model.num_properties() >= 1ul);
	REQUIRE(model.num_simulators() > 0ul);
	REQUIRE(model.num_importance_functions() > 0ul);
	REQUIRE(model.num_importance_strategies() > 0ul);
	REQUIRE(model.num_threshold_techniques() > 0ul);
	REQUIRE(model.num_RNGs() > 0ul);
}

SECTION("M3 without resampling (transient, RESTART, compositional (DFT), ad hoc thresholds)")
{
	const string nameEngine("restart");
	const string ifunCompDFT("BE_0+max(BE_1,BE_2)+BE_4");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", ifunCompDFT,
	                               fig::PostProcessing(), 0, 3);
	const string thrAdHoc("1:3,2:6");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(false);
	model.set_DFT(true);
	model.build_importance_function_auto(ifunSpec, propId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, propId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 909250341ul);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(M3_SECONDS));
	// Estimate
	model.estimate(propId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(M3_PROB).epsilon(M3_PROB*.3));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(M3_PROB*.4).epsilon(M3_PROB*.2));
	model.set_DFT(false);
}

SECTION("M3 with resampling (transient, RESTART, compositional (DFT), ad hoc thresholds)")
{
	const string nameEngine("restart");
	const string ifunCompDFT("BE_0+max(BE_1,BE_2)+BE_4");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", ifunCompDFT,
								   fig::PostProcessing(), 0, 3);
	const string thrAdHoc("1:3,2:6");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(true);
	model.set_DFT(true);
	model.build_importance_function_auto(ifunSpec, propId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, propId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 909250341ul);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(M3_SECONDS));
	// Estimate
	model.estimate(propId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(M3_PROB).epsilon(M3_PROB*.3));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(M3_PROB*.4).epsilon(M3_PROB*.2));
	model.set_DFT(false);
}

SECTION("M3 with resampling (transient, Fixed Effort, compositional (DFT), ad hoc thresholds)")
{
	const string nameEngine("sfe");
	const string ifunCompDFT("BE_0+max(BE_1,BE_2)+BE_4");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", ifunCompDFT,
								   fig::PostProcessing(), 0, 3);
	const string thrAdHoc("1:11,2:16");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(true);
	model.set_DFT(true);
	model.build_importance_function_auto(ifunSpec, propId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, propId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	REQUIRE(model.exists_rng("mt64"));
	model.set_rng("mt64", std::mt19937_64::default_seed);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(M3_SECONDS));
	// Estimate
	model.estimate(propId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(M3_PROB).epsilon(M3_PROB*.3));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(M3_PROB*.4).epsilon(M3_PROB*.2));
	model.set_DFT(false);
}

SECTION("Reset default value of resampling for all engines")
{
	model.set_resampling();
}

} // TEST_CASE [resampling]

} // namespace tests   // // // // // // // // // // // // // // // // // // //
