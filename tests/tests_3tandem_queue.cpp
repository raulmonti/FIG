//==============================================================================
//
//  tests_3tandem_queue.cpp
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
const string MODEL(tests::models_dir() + "3tandem_queue.sa");

// TAD which will contain the compiled model
fig::ModelSuite& model(fig::ModelSuite::get_instance());

// Steady-state query: S ( q3 >= 7 )
const double SS_PROB(4.25e-6);  // expected result of steady-state query
int ssPropId(-1);               // index of the query within our TAD

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

TEST_CASE("Triple tandem queue tests", "[3-tandem-queue]")
{

SECTION("Compile model file")
{
    preamble_testcase(fig::figTechLog, "3-tandem-queue");

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

	// Register one steady-state property
	ssPropId = -1;
	REQUIRE(model.num_properties() >= 1ul);
	for (size_t i = 0ul ; i < model.num_properties() ; i++) {
		auto prop = model.get_property(i);
		if (prop->type == fig::PropertyType::RATE) {
			ssPropId = i;
			break;
		}
	}
	REQUIRE(0 <= ssPropId);
	REQUIRE(nullptr != model.get_property(ssPropId));
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
/*
SECTION("Steady-state: standard MC")
{
	const string nameEngine("nosplit");
	const string nameIFun("algebraic");
	const string nameThr("fix");
	const string rng("pcg32");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(nameIFun));
	REQUIRE(model.exists_threshold_technique(nameThr));
	REQUIRE(model.exists_rng(rng));
	// Prepare engine
	model.build_importance_function_flat(nameIFun, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, nameIFun, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	model.set_rng(rng);
	const double confCo(.7);
	const double prec(.6);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
    model.set_timeout(TIMEOUT_(std::chrono::minutes(1)));  // don't waste too much time...
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, fig::ImpFunSpec(nameIFun, "flat"));
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*prec));
	REQUIRE(ci.precision(.9) > 0.0);
	REQUIRE(ci.precision(.9) < SS_PROB);
}

SECTION("Steady-state: RESTART, ad hoc, es")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("algebraic", "adhoc", "q3");
	const string nameThr("es");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.build_importance_function_adhoc(ifunSpec, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 1234567890987ul);
	const double confCo(.95);
	const double prec(.2);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(TIMEOUT_(0));  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*0.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  == Approx(SS_PROB*prec).epsilon(SS_PROB*0.2));
}
*/
SECTION("Steady-state: RESTART-P0, ad hoc, ad hoc thresholds")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("algebraic", "adhoc", "q1+2*q2+5*q3");
	//const string nameThr("fix");
	const string thrAdHoc("21:2,24:2,26:2,27:2,30:3,34:3,37:2,39:3,40:2,42:2");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_global_effort(3);
	model.build_importance_function_adhoc(ifunSpec, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, ssPropId);
	//auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 314159265ul);
	const double confCo(.95);
	const double prec(.3);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(TIMEOUT_(0));  // unset timeout; estimate for as long as necessary
	//model.set_timeout(TIMEOUT_(6));  /// @todo TODO erase
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*0.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(SS_PROB*prec).epsilon(SS_PROB*0.25));
}

SECTION("Steady-state: RESTART-P3, ad hoc, ad hoc thresholds")
{
	const string nameEngine("restart3");
	const fig::ImpFunSpec ifunSpec("algebraic", "adhoc", "q1+2*q2+5*q3");
	//const string nameThr("fix");
	const string thrAdHoc("21:2,24:2,26:2,27:2,30:3,34:3,37:2,39:3,40:2,42:2");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	// Prepare engine
	model.set_global_effort(3);
	model.build_importance_function_adhoc(ifunSpec, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, thrAdHoc, ssPropId);
	//auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 314159265ul);
	const double confCo(.95);
	const double prec(.3);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
    model.set_timeout(TIMEOUT_(0));  // unset timeout; estimate for as long as necessary
	//model.set_timeout(TIMEOUT_(12));  /// @todo TODO erase
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*0.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(SS_PROB*prec).epsilon(SS_PROB*0.25));
}
/*
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
	model.set_global_effort(8, nameEngine);
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
    model.set_timeout(TIMEOUT_(std::chrono::minutes(2)));  // estimate for 2 min max
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*0.4));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          <= Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
}

SECTION("Steady-state: RESTART, compositional, es")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "max");
	const string nameThr("es");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0);
	const double confCo(.95);
	const double prec(.35);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
    model.set_timeout(TIMEOUT_(0));  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*0.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          <= Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
}
*/

} // TEST_CASE [triple-tandem-queue]

} // namespace tests   // // // // // // // // // // // // // // // // // // //
