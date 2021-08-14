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

// Model file (full path to)
const string MODEL(tests::models_dir() + "tandem_queue.sa");

// TAD which will contain the compiled model
fig::ModelSuite& model(fig::ModelSuite::get_instance());

// Transient query: P ( q2 > 0 U q2 == 8 )
const double TR_PROB(5.59e-6);  // expected result of transient query
int trPropId(-1);               // index of the query within our TAD

// Steady-state query: S ( q2 == 8 )
const double SS_PROB(6.23e-5);  // expected result of steady-state query (C=10: 7.25e-6)
int ssPropId(-1);               // index of the query within our TAD

// Time bounded steady-state query: S[ 60:30000 ]( q2 == 8 )
const double BS_PROB(6.15e-5);  // expected result of steady-state query
int bsPropId(-1);               // index of the query within our TAD

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

TEST_CASE("Tandem queue tests", "[tandem-queue]")
{

SECTION("Compile model file")
{
    preamble_testcase(fig::figTechLog, "tandem-queue");

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

	// Register one transient and one steady-state property
	trPropId = -1;
	ssPropId = -1;
	bsPropId = -1;
	REQUIRE(model.num_properties() >= 3ul);
	for (auto i = 0ul ; i < model.num_properties() ; i++) {
		auto prop = model.get_property(i);
		if (prop->type == fig::PropertyType::TRANSIENT)
			trPropId = static_cast<decltype(trPropId)>( i );
		else if (prop->type == fig::PropertyType::RATE)
			ssPropId = static_cast<decltype(ssPropId)>( i );
		else if (prop->type == fig::PropertyType::TBOUNDED_SS)
			bsPropId = static_cast<decltype(bsPropId)>( i );
		if (0 <= trPropId && 0 <= ssPropId && 0 <= bsPropId)
			break;  // already got one of each kind
	}
	REQUIRE(0 <= trPropId);
	REQUIRE(0 <= ssPropId);
	REQUIRE(0 <= bsPropId);
	REQUIRE(nullptr != model.get_property(trPropId));
	REQUIRE(nullptr != model.get_property(ssPropId));
	REQUIRE(nullptr != model.get_property(bsPropId));
}

SECTION("Seal model and check consistency")
{
	REQUIRE(seal_model());
	REQUIRE(model.num_modules() > 0ul);
	REQUIRE(model.num_properties() >= 3ul);
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
	//model.set_global_effort(1);
	model.build_importance_function_flat(nameIFun, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, nameIFun, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 8);
	fig::StoppingConditions timeBound;
	timeBound.add_time_budget(10);  // estimate for 10 seconds
	// Estimate
	model.estimate(trPropId, *engine, timeBound, fig::ImpFunSpec(nameIFun, "flat"));
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.8));
	REQUIRE(ci.precision(.9) > 0.0);
	REQUIRE(ci.precision(.9) < TR_PROB*1.5);
	model.release_resources(nameIFun, nameEngine);
}

SECTION("Steady-state: RESTART, ad hoc, hyb")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("algebraic", "adhoc", "q2");
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_global_effort(3, nameEngine);
	model.build_importance_function_adhoc(ifunSpec, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 12);
	const double confCo(.95);
	const double prec(.4);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
    model.set_timeout(TIMEOUT_(0));  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.2));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  == Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
	model.release_resources(ifunSpec.name, nameEngine);
}

SECTION("Steady-state: RESTART, monolithic, hyb")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_coupled", "auto");
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.release_resources(ifunSpec.name);
	model.set_global_effort(5, nameEngine);
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 42);
	const double confCo(.95);
	const double prec(.3);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(TIMEOUT_(1)));  // estimate for a min max
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.6));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
	model.release_resources(ifunSpec.name, nameEngine);
}

SECTION("Time bounded steady-state: RESTART, compositional (+ operator), fix")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+");
	const string nameThr("fix");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.release_resources(ifunSpec.name);
	model.set_global_effort(3, nameEngine);
	model.build_importance_function_auto(ifunSpec, bsPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, bsPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 314159265);
	const double confCo(.95);
	const double prec(.1);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(TIMEOUT_(45));
	// Estimate
	model.estimate(bsPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(BS_PROB).epsilon(BS_PROB*.5));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(BS_PROB*prec).epsilon(BS_PROB*.3));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  == Approx(BS_PROB*prec).epsilon(BS_PROB*0.15));
	model.release_resources(ifunSpec.name, nameEngine);
}


SECTION("Time bounded steady-state: RESTART-P2, compositional (+ operator), fix")
{
	const string nameEngine("restart2");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+");
	const string nameThr("fix");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.release_resources(ifunSpec.name);
	model.set_global_effort(3, nameEngine);
	model.build_importance_function_auto(ifunSpec, bsPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, bsPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 314159265);
	const double confCo(.95);
	const double prec(.1);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(TIMEOUT_(45));
	//model.set_timeout(TIMEOUT_(9));  /// @todo TODO erase
	// Estimate
	model.estimate(bsPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(BS_PROB).epsilon(BS_PROB*.5));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(BS_PROB*prec).epsilon(BS_PROB*.3));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(BS_PROB*prec).epsilon(BS_PROB*0.15));
	model.release_resources(ifunSpec.name, nameEngine);
}

SECTION("Time bounded steady-state: RESTART-P1, ad hoc, ad hoc thresholds")
{
	const string nameEngine("restart1");
	const fig::ImpFunSpec ifunSpec("algebraic", "adhoc", "q1+3*q2");
	const string thrAdHoc("[(10:2),(13:2),(16:2),(19:3),(22:3),(25:2),(28:2),(31:3)]");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.release_resources(ifunSpec.name);
	model.set_global_effort(0, nameEngine);
	model.build_importance_function_adhoc(ifunSpec, bsPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, bsPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 122333);
	const double confCo(.95);
	const double prec(.05);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(TIMEOUT_(2)));
	// Estimate
	model.set_verbosity(true);
	model.estimate(bsPropId, *engine, confCrit, ifunSpec);
	model.set_verbosity(false);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(BS_PROB).epsilon(BS_PROB*.4));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(BS_PROB*prec).epsilon(BS_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(BS_PROB*prec).epsilon(BS_PROB*0.2));
	model.release_resources(ifunSpec.name, nameEngine);
}

SECTION("Time bounded steady-state: RESTART, monolithic, ad hoc thresholds")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_coupled", "auto");
	const string thrAdHoc("[(4:2),(6:2),(8:2),(10:2),(11:2),(12:2),(13:2)]");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.build_importance_function_auto(ifunSpec, bsPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, bsPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 333221);
	const double confCo(.95);
	const double prec(.15);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(TIMEOUT_(45));
	// Estimate
	model.set_verbosity(true);
	model.estimate(bsPropId, *engine, confCrit, ifunSpec);
	model.set_verbosity(false);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(BS_PROB).epsilon(BS_PROB*.3));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(BS_PROB*prec).epsilon(BS_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(BS_PROB*prec).epsilon(BS_PROB*0.1));
	model.release_resources(ifunSpec.name, nameEngine);
}

SECTION("Transient: RESTART, compositional (+ operator), es")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+");
	const string nameThr("es");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 127);
	const double confCo(.97);
	const double prec(.2);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(TIMEOUT_(2)));
	// Estimate
	model.estimate(trPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.3));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(TR_PROB*prec).epsilon(TR_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(TR_PROB*prec).epsilon(TR_PROB*0.1));
	model.release_resources(ifunSpec.name, nameEngine);
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
	model.set_global_effort(5, nameEngine);
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 126);
	const double confCo(.95);
	const double prec(.35);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
    model.set_timeout(TIMEOUT_(0));  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(trPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.3));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(TR_PROB*prec).epsilon(TR_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(TR_PROB*prec).epsilon(TR_PROB*0.1));
	model.release_resources(ifunSpec.name, nameEngine);
}

SECTION("Transient: Fixed Effort, compositional (max operator), es")
{
	const string nameEngine("sfe");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "max");
	const string nameThr("es");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.build_importance_function_auto(ifunSpec, trPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, trPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 126);
	const double confCo(.95);
	const double prec(.35);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
    model.set_timeout(TIMEOUT_(0));  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(trPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(TR_PROB).epsilon(TR_PROB*.3));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(TR_PROB*prec).epsilon(TR_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(TR_PROB*prec).epsilon(TR_PROB*0.1));
}

} // TEST_CASE [tandem-queue]

} // namespace tests   // // // // // // // // // // // // // // // // // // //
