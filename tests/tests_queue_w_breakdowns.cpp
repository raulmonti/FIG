//==============================================================================
//
//  tests_queue_w_breakdowns.cpp
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

// Model file (full path to)
const string MODEL(tests::models_dir() + "queue_w_breakdowns.sa");

// TAD which will contain the compiled model
fig::ModelSuite& model(fig::ModelSuite::get_instance());

// Transient query: P ( !reset U buf == 50 )
constexpr double TR_PROB = 7.743994e-5;  // from PRISM
int trPropId(-1);  // index of the query within our TAD

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

TEST_CASE("Queue with breakdowns tests", "[queue-w-breakdowns]")
{

SECTION("Compile model file")
{
    preamble_testcase(fig::figTechLog, "queue-w-breakdowns");

    // If this is not the first test then we need to clean
	// the ModelSuite singleton before loading the new model
	if (model.sealed())
		model.clear();
	REQUIRE_FALSE(model.sealed());
	REQUIRE(compile_model(MODEL));
	// XXX ModelSuite is a singleton: the compiled model is available
	//                                to all following SECTION blocks
	REQUIRE_FALSE(model.sealed());
	CHECK(model.num_modules() > 0ul);

	// Register one transient property
	trPropId = -1;
	REQUIRE(model.num_properties() >= 1ul);
	for (size_t i = 0ul ; i < model.num_properties() ; i++) {
		auto prop = model.get_property(i);
		if (prop->type == fig::PropertyType::TRANSIENT) {
			trPropId = i;
			break;
		}
	}
	REQUIRE(0 <= trPropId);
	REQUIRE(nullptr != model.get_property(trPropId));
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

SECTION("Transient: standard MC")
{
	const string nameEngine("nosplit");
	const string nameIFun("algebraic");
	const string nameThr("fix");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(nameIFun));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.build_importance_function_flat(nameIFun, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, nameIFun, nameThr, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng);
	const double confCo(.9);
	const double prec(.2);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(TIMEOUT_(90));  // truncate sims after 1.5 min
	// Estimate
	model.estimate(trPropId, *engine, confCrit, fig::ImpFunSpec(nameIFun, "flat"));
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.8));
	REQUIRE(ci.precision(.9) > 0.0);
	REQUIRE(ci.precision(.9) < TR_PROB*1.6);
}

SECTION("Transient: RESTART, ad hoc, es")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("algebraic", "adhoc", "buf");
	const string nameThr("es");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_resampling(false);
	model.set_global_effort(2, nameEngine);
	model.build_importance_function_adhoc(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 911);
	const double confCo(.95);
	const double prec(.05);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(TIMEOUT_(90));  // truncate sims after 1.5 min
	// Estimate
	model.estimate(trPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.4));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(TR_PROB*prec).epsilon(TR_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  == Approx(TR_PROB*prec).epsilon(TR_PROB*.6));
}

SECTION("Transient: Fixed Effort, monolithic, hyb")
{
    const string nameEngine("sfe");
	const fig::ImpFunSpec ifunSpec("concrete_coupled", "auto");
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_resampling(true);
	model.set_global_effort(4, nameEngine);
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, std::mt19937_64::default_seed);
	fig::StoppingConditions timeBound;
    timeBound.add_time_budget(TIMEOUT_(90));  // estimate for 1.5 min
	// Estimate
	model.estimate(trPropId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.8));
	REQUIRE(ci.precision(.8) > 0.0);
	REQUIRE(ci.precision(.8) <= Approx(TR_PROB).epsilon(TR_PROB*.2));
}

SECTION("Transient: RESTART, compositional (max), es")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "max");
	const string nameThr("es");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_resampling(false);
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 666);
	const double confCo(.95);
	const double prec(.11);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(TIMEOUT_(2)));  // truncate sims after 2 min
	// Estimate
	model.estimate(trPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(TR_PROB*prec).epsilon(TR_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  == Approx(TR_PROB*prec).epsilon(TR_PROB*.6));
}

SECTION("Transient: Fixed Effort, compositional (+), hyb")
{
	const string nameEngine("sfe");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+");
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_resampling(true);
	model.set_global_effort(3, nameEngine);
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 666);
	const double confCo(.95);
	const double prec(.11);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(TIMEOUT_(2)));  // truncate sims after 2 min
	// Estimate
	model.estimate(trPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(TR_PROB*prec).epsilon(TR_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  == Approx(TR_PROB*prec).epsilon(TR_PROB*.6));
}

SECTION("Transient: no resampling, RESTART, compositional (+), ad hoc thresholds")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+");
	const string thrAdHoc("3:2,6:2,9:2,13:2,17:2,20:2,24:2,28:2,33:2,39:2,43:2,48:2");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(false, "restart");
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 590433664ul);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(90));
	// Estimate
	model.estimate(trPropId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.3));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(TR_PROB*.5).epsilon(TR_PROB*.2));
}

SECTION("Transient: resampling, RESTART, compositional (+), ad hoc thresholds")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+");
	const string thrAdHoc("3:2,6:2,9:2,13:2,17:2,20:2,24:2,28:2,33:2,39:2,43:2,48:2");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(true, "restart");
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 590433664ul);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(90));
	// Estimate
	model.estimate(trPropId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.3));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(TR_PROB*.5).epsilon(TR_PROB*.2));
}

SECTION("Transient: no resampling, Fixed Effort, compositional (+), ad hoc thresholds")
{
	const string nameEngine("sfe");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+");
	const string thrAdHoc("3:3,4:3,7:3,8:3,9:3,10:3,11:3,12:3,13:3,14:3,18:3,24:3,25:3,27:3,31:3,43:3,45:3,47:3,49:3");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(false, "sfe");
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	REQUIRE(model.exists_rng("mt64"));
	model.set_rng("mt64", std::mt19937_64::default_seed);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(90));
	// Estimate
	model.estimate(trPropId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.3));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(TR_PROB*.5).epsilon(TR_PROB*.2));
}

SECTION("Transient: resampling, Fixed Effort, compositional (+), ad hoc thresholds")
{
	const string nameEngine("sfe");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+");
	const string thrAdHoc("3:3,4:3,7:3,8:3,9:3,10:3,11:3,12:3,13:3,14:3,18:3,24:3,25:3,27:3,31:3,43:3,45:3,47:3,49:3");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_resampling(true, "sfe");
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	REQUIRE(model.exists_rng("mt64"));
	model.set_rng("mt64", std::mt19937_64::default_seed);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(TIMEOUT_(90));
	// Estimate
	model.estimate(trPropId, *engine, timeBound, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.3));
	REQUIRE(ci.precision(.1) > 0.0);
	REQUIRE(ci.precision(.1) <= Approx(TR_PROB*.5).epsilon(TR_PROB*.2));
}

SECTION("Reset default value of resampling for all engines")
{
	model.set_resampling();
}

} // TEST_CASE [queue-w-breakdowns]

} // namespace tests   // // // // // // // // // // // // // // // // // // //
