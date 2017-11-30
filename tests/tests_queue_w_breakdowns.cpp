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
const double TR_PROB(7.53e-5);  // expected result of transient query
int trPropId(-1);               // index of the query within our TAD

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

TEST_CASE("Queue with breakdowns tests", "[queue-w-breakdowns]")
{

SECTION("Compile model file")
{
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
	model.set_global_effort(1);
	model.build_importance_function_flat(nameIFun, trPropId, true);
	model.build_thresholds(nameThr, nameIFun, trPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, nameIFun);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng);
	const double confCo(.77);
	const double prec(.8);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	// Estimate
	model.estimate(trPropId, *engine, confCrit);
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
	model.set_global_effort();
	model.build_importance_function_adhoc(ifunSpec, trPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, trPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 911);
	const double confCo(.95);
	const double prec(.4);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(0);  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(trPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.4));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(TR_PROB*prec).epsilon(TR_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(TR_PROB*prec).epsilon(TR_PROB*0.1));
}

SECTION("Transient: RESTART, monolithic, hyb")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_coupled", "auto");
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_global_effort(2);
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, trPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, std::mt19937_64::default_seed);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(30);  // estimate for 30 seconds
	// Estimate
	model.estimate(trPropId, *engine, timeBound);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.8));
	REQUIRE(ci.precision(.8) > 0.0);
	REQUIRE(ci.precision(.8) <= Approx(TR_PROB*.5).epsilon(TR_PROB*0.4));
}

SECTION("Transient: RESTART, compositional (max operator), es")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "max");
	const string nameThr("es");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, trPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 666);
	const double confCo(.95);
	const double prec(.4);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	// Estimate
	model.estimate(trPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(TR_PROB*prec).epsilon(TR_PROB*0.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(TR_PROB*prec).epsilon(TR_PROB*0.2));
}

SECTION("Transient: Fixed Effort, compositional (+ operator), hyb")
{
	const string nameEngine("fixedeffort");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+");
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_global_effort(3);
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, trPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 666);
	const double confCo(.95);
	const double prec(.4);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	// Estimate
	model.estimate(trPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(TR_PROB*prec).epsilon(TR_PROB*0.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(TR_PROB*prec).epsilon(TR_PROB*0.2));
}

} // TEST_CASE [queue-w-breakdowns]

} // namespace tests   // // // // // // // // // // // // // // // // // // //
