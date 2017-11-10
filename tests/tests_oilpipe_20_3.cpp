//==============================================================================
//
//  tests_oilpipes_20_3.cpp
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

// Model files (full path to)
const string MODEL_EXP(tests::models_dir() + "oilpipe_20_3_exp.sa");
const string MODEL_RAY(tests::models_dir() + "oilpipe_20_3_ray.sa");

// TAD which will contain the compiled model
fig::ModelSuite& model(fig::ModelSuite::get_instance());

// Steady-state query:
// S((broken_pipe1>0 & broken_pipe2>0 & broken_pipe3>0)    |
//   (broken_pipe2>0 & broken_pipe3>0 & broken_pipe4>0)    |
//   (broken_pipe3>0 & broken_pipe4>0 & broken_pipe5>0)    |
//   (broken_pipe4>0 & broken_pipe5>0 & broken_pipe6>0)    |
//   (broken_pipe5>0 & broken_pipe6>0 & broken_pipe7>0)    |
//   (broken_pipe6>0 & broken_pipe7>0 & broken_pipe8>0)    |
//   (broken_pipe7>0 & broken_pipe8>0 & broken_pipe9>0)    |
//   (broken_pipe8>0 & broken_pipe9>0 & broken_pipe10>0)   |
//   (broken_pipe9>0 & broken_pipe10>0 & broken_pipe11>0)  |
//   (broken_pipe10>0 & broken_pipe11>0 & broken_pipe12>0) |
//   (broken_pipe11>0 & broken_pipe12>0 & broken_pipe13>0) |
//   (broken_pipe12>0 & broken_pipe13>0 & broken_pipe14>0) |
//   (broken_pipe13>0 & broken_pipe14>0 & broken_pipe15>0) |
//   (broken_pipe14>0 & broken_pipe15>0 & broken_pipe16>0) |
//   (broken_pipe15>0 & broken_pipe16>0 & broken_pipe17>0) |
//   (broken_pipe16>0 & broken_pipe17>0 & broken_pipe18>0) |
//   (broken_pipe17>0 & broken_pipe18>0 & broken_pipe19>0) |
//   (broken_pipe18>0 & broken_pipe19>0 & broken_pipe20>0) )
const double SS_PROB_EXP(1.53e-5);  // expected result of steady-state query in EXP model
const double SS_PROB_RAY(2.02e-5);  // expected result of steady-state query in RAY model
int ssPropId(-1);                   // index of the query within our TAD

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

TEST_CASE("Oil pipeline (EXP) tests, for N=20 and K=3", "[oilpipes-EXP-N20-K3]")
{

SECTION("Compile model file")
{
	// If this is not the first test then we need to clean
	// the ModelSuite singleton before loading the new model
	if (model.sealed())
		model.clear();
	REQUIRE_FALSE(model.sealed());
	REQUIRE(compile_model(MODEL_EXP));
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

SECTION("Estimate steady-state property using standard MC")
{
	const string nameEngine("nosplit");
	const string nameIFun("algebraic");
	const string nameThr("fix");
	const string rng("pcg64");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(nameIFun));
	REQUIRE(model.exists_threshold_technique(nameThr));
	REQUIRE(model.exists_rng(rng));
	// Prepare engine
	model.set_splitting(1);
	model.build_importance_function_flat(nameIFun, ssPropId, true);
	model.build_thresholds(nameThr, nameIFun, ssPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, nameIFun);
	REQUIRE(engine->ready());
	// Set estimation criteria
	model.set_rng(rng, 0ul);
	const double confCo(.8);
	const double prec(.4);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(20);  // don't estimate for that long a time
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB_EXP).epsilon(SS_PROB_EXP*prec*2.0));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) < SS_PROB_EXP);
}

SECTION("Estimate steady-state property using RESTART and adhoc ifun")
{
	const string nameEngine("restart");
	const string ifunAdHoc("max((broken_pipe1>0)+(broken_pipe2>0)+(broken_pipe3>0),(broken_pipe2>0)+(broken_pipe3>0)+(broken_pipe4>0),(broken_pipe3>0)+(broken_pipe4>0)+(broken_pipe5>0),(broken_pipe4>0)+(broken_pipe5>0)+(broken_pipe6>0),(broken_pipe5>0)+(broken_pipe6>0)+(broken_pipe7>0),(broken_pipe6>0)+(broken_pipe7>0)+(broken_pipe8>0),(broken_pipe7>0)+(broken_pipe8>0)+(broken_pipe9>0),(broken_pipe8>0)+(broken_pipe9>0)+(broken_pipe10>0),(broken_pipe9>0)+(broken_pipe10>0)+(broken_pipe11>0),(broken_pipe10>0)+(broken_pipe11>0)+(broken_pipe12>0),(broken_pipe11>0)+(broken_pipe12>0)+(broken_pipe13>0),(broken_pipe12>0)+(broken_pipe13>0)+(broken_pipe14>0),(broken_pipe13>0)+(broken_pipe14>0)+(broken_pipe15>0),(broken_pipe14>0)+(broken_pipe15>0)+(broken_pipe16>0),(broken_pipe15>0)+(broken_pipe16>0)+(broken_pipe17>0),(broken_pipe16>0)+(broken_pipe17>0)+(broken_pipe18>0),(broken_pipe17>0)+(broken_pipe18>0)+(broken_pipe19>0),(broken_pipe18>0)+(broken_pipe19>0)+(broken_pipe20>0),0)");
	const fig::ImpFunSpec ifunSpec("algebraic", "adhoc",
	                               ifunAdHoc,
	                               fig::PostProcessing(),
	                               0, 3);
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_splitting(3);
	model.build_importance_function_adhoc(ifunSpec, ssPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, ssPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng);
	const double confCo(.95);
	const double prec(.3);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(0);  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB_EXP).epsilon(SS_PROB_EXP*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB_EXP*prec).epsilon(SS_PROB_EXP*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(SS_PROB_EXP*prec).epsilon(SS_PROB_EXP*.1));
}

SECTION("Estimate steady-state property using RESTART and compositional ifun (op:*)")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "*",
								   fig::PostProcessing(fig::PostProcessing::EXP, "exp", 2.0));
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_splitting(4);
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, ssPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0);
	const double confCo(.95);
	const double prec(.3);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(0);  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB_EXP).epsilon(SS_PROB_EXP*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB_EXP*prec).epsilon(SS_PROB_EXP*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  == Approx(SS_PROB_EXP*prec).epsilon(SS_PROB_EXP*0.1));
}

} // TEST_CASE [oilpipes-EXP-N20-K3]


TEST_CASE("Oil pipeline (RAY) tests, for N=20 and K=3", "[oilpipes-RAY-N20-K3]")
{

SECTION("Compile model file")
{
	// If this is not the first test then we need to clean
	// the ModelSuite singleton before loading the new model
	if (model.sealed())
		model.clear();
	REQUIRE_FALSE(model.sealed());
	REQUIRE(compile_model(MODEL_RAY));
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

SECTION("Estimate steady-state property using standard MC")
{
	const string nameEngine("nosplit");
	const string nameIFun("algebraic");
	const string nameThr("fix");
	const string rng("mt64");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(nameIFun));
	REQUIRE(model.exists_threshold_technique(nameThr));
	REQUIRE(model.exists_rng(rng));
	// Prepare engine
	model.set_splitting(1);
	model.build_importance_function_flat(nameIFun, ssPropId, true);
	model.build_thresholds(nameThr, nameIFun, ssPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, nameIFun);
	REQUIRE(engine->ready());
	// Set estimation criteria
	model.set_rng(rng, 0ul);
	const double confCo(.8);
	const double prec(.4);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(30);  // don't estimate for that long a time
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB_EXP).epsilon(SS_PROB_EXP*prec*2.0));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) < SS_PROB_EXP);
}

SECTION("Estimate steady-state property using RESTART and compositional ifun (max,+)")
{
	const string nameEngine("restart");
	const string ifunComp("max(BE_pipe1+BE_pipe2+BE_pipe3,BE_pipe2+BE_pipe3+BE_pipe4,BE_pipe3+BE_pipe4+BE_pipe5,BE_pipe4+BE_pipe5+BE_pipe6,BE_pipe5+BE_pipe6+BE_pipe7,BE_pipe6+BE_pipe7+BE_pipe8,BE_pipe7+BE_pipe8+BE_pipe9,BE_pipe8+BE_pipe9+BE_pipe10,BE_pipe9+BE_pipe10+BE_pipe11,BE_pipe10+BE_pipe11+BE_pipe12,BE_pipe11+BE_pipe12+BE_pipe13,BE_pipe12+BE_pipe13+BE_pipe14,BE_pipe13+BE_pipe14+BE_pipe15,BE_pipe14+BE_pipe15+BE_pipe16,BE_pipe15+BE_pipe16+BE_pipe17,BE_pipe16+BE_pipe17+BE_pipe18,BE_pipe17+BE_pipe18+BE_pipe19,BE_pipe18+BE_pipe19+BE_pipe20,0)");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto",
								   ifunComp,
								   fig::PostProcessing(),
								   0, 3);
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_splitting(3);
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, ssPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 111ul);
	const double confCo(.95);
	const double prec(.3);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(0);  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB_EXP).epsilon(SS_PROB_EXP*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB_EXP*prec).epsilon(SS_PROB_EXP*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  <= Approx(SS_PROB_EXP*prec).epsilon(SS_PROB_EXP*.3));
}

SECTION("Estimate steady-state property using RESTART and compositional ifun (+,*)")
{
	const string nameEngine("restart");
	const string ifunComp("BE_pipe1*BE_pipe2*BE_pipe3+BE_pipe2*BE_pipe3*BE_pipe4+BE_pipe3*BE_pipe4*BE_pipe5+BE_pipe4*BE_pipe5*BE_pipe6+BE_pipe5*BE_pipe6*BE_pipe7+BE_pipe6*BE_pipe7*BE_pipe8+BE_pipe7*BE_pipe8*BE_pipe9+BE_pipe8*BE_pipe9*BE_pipe10+BE_pipe9*BE_pipe10*BE_pipe11+BE_pipe10*BE_pipe11*BE_pipe12+BE_pipe11*BE_pipe12*BE_pipe13+BE_pipe12*BE_pipe13*BE_pipe14+BE_pipe13*BE_pipe14*BE_pipe15+BE_pipe14*BE_pipe15*BE_pipe16+BE_pipe15*BE_pipe16*BE_pipe17+BE_pipe16*BE_pipe17*BE_pipe18+BE_pipe17*BE_pipe18*BE_pipe19+BE_pipe18*BE_pipe19*BE_pipe20");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto",
								   ifunComp,
								   fig::PostProcessing(fig::PostProcessing::EXP, "exp", 2.0),
								   18, 144);
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_splitting(4);
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, ssPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 3444057033415490ul);
	const double confCo(.95);
	const double prec(.3);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(0);  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB_EXP).epsilon(SS_PROB_EXP*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB_EXP*prec).epsilon(SS_PROB_EXP*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  <= Approx(SS_PROB_EXP*prec).epsilon(SS_PROB_EXP*.3));
}

} // TEST_CASE [oilpipes-RAY-N20-K3]


} // namespace tests   // // // // // // // // // // // // // // // // // // //
