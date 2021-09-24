//==============================================================================
//
//  tests_database_r3.cpp
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

// Model file (full path to)
const string MODEL(tests::models_dir() + "database_r3.sa");

// TAD which will contain the compiled model
fig::ModelSuite& model(fig::ModelSuite::get_instance());

// Steady-state query:
//	S( (d11f & d12f & d13f)
//	 | (d11f & d12f & d14f)
//	 | (d11f & d12f & d15f)
//	 | (d11f & d13f & d14f)
//	 | (d11f & d13f & d15f)
//	 | (d11f & d14f & d15f)
//	 | (d12f & d13f & d14f)
//	 | (d12f & d13f & d15f)
//	 | (d12f & d14f & d15f)
//	 | (d13f & d14f & d15f)
//	 | (d21f & d22f & d23f)
//	 | (d21f & d22f & d24f)
//	 | (d21f & d22f & d25f)
//	 | (d21f & d23f & d24f)
//	 | (d21f & d23f & d25f)
//	 | (d21f & d24f & d25f)
//	 | (d22f & d23f & d24f)
//	 | (d22f & d23f & d25f)
//	 | (d22f & d24f & d25f)
//	 | (d23f & d24f & d25f)
//	 | (d31f & d32f & d33f)
//	 | (d31f & d32f & d34f)
//	 | (d31f & d32f & d35f)
//	 | (d31f & d33f & d34f)
//	 | (d31f & d33f & d35f)
//	 | (d31f & d34f & d35f)
//	 | (d32f & d33f & d34f)
//	 | (d32f & d33f & d35f)
//	 | (d32f & d34f & d35f)
//	 | (d33f & d34f & d35f)
//	 | (d41f & d42f & d43f)
//	 | (d41f & d42f & d44f)
//	 | (d41f & d42f & d45f)
//	 | (d41f & d43f & d44f)
//	 | (d41f & d43f & d45f)
//	 | (d41f & d44f & d45f)
//	 | (d42f & d43f & d44f)
//	 | (d42f & d43f & d45f)
//	 | (d42f & d44f & d45f)
//	 | (d43f & d44f & d45f)
//	 | (d51f & d52f & d53f)
//	 | (d51f & d52f & d54f)
//	 | (d51f & d52f & d55f)
//	 | (d51f & d53f & d54f)
//	 | (d51f & d53f & d55f)
//	 | (d51f & d54f & d55f)
//	 | (d52f & d53f & d54f)
//	 | (d52f & d53f & d55f)
//	 | (d52f & d54f & d55f)
//	 | (d53f & d54f & d55f)
//	 | (d61f & d62f & d63f)
//	 | (d61f & d62f & d64f)
//	 | (d61f & d62f & d65f)
//	 | (d61f & d63f & d64f)
//	 | (d61f & d63f & d65f)
//	 | (d61f & d64f & d65f)
//	 | (d62f & d63f & d64f)
//	 | (d62f & d63f & d65f)
//	 | (d62f & d64f & d65f)
//	 | (d63f & d64f & d65f)
//	 | (c11f & c12f & c13f)
//	 | (c21f & c22f & c23f)
//	 | (p11f & p12f & p13f)
//	 | (p21f & p22f & p23f) )
const double SS_PROB(4.74e-5);  // expected result of steady-state query
int ssPropId(-1);               // index of the query within our TAD

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

TEST_CASE("Database with R=3 tests", "[database-R3]")
{

SECTION("Compile model file")
{
	preamble_testcase(fig::figTechLog, "database-R3");

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
	model.set_rng(rng, 0ul);
	const double confCo(.9);
	const double prec(.3);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(TIMEOUT_(1)));  // estimate for 1 min max
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, fig::ImpFunSpec(nameIFun, "flat"));
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.2));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) < SS_PROB);
}

SECTION("Steady-state: RESTART, ad hoc, hyb")
{
	const string nameEngine("restart");
	const string ifunAdHoc("max(d11f+d12f+d13f,max(d11f+d12f+d14f,max(d11f+d12f+d15f,max(d11f+d13f+d14f,max(d11f+d13f+d15f,max(d11f+d14f+d15f,max(d12f+d13f+d14f,max(d12f+d13f+d15f,max(d12f+d14f+d15f,max(d13f+d14f+d15f,max(d21f+d22f+d23f,max(d21f+d22f+d24f,max(d21f+d22f+d25f,max(d21f+d23f+d24f,max(d21f+d23f+d25f,max(d21f+d24f+d25f,max(d22f+d23f+d24f,max(d22f+d23f+d25f,max(d22f+d24f+d25f,max(d23f+d24f+d25f,max(d31f+d32f+d33f,max(d31f+d32f+d34f,max(d31f+d32f+d35f,max(d31f+d33f+d34f,max(d31f+d33f+d35f,max(d31f+d34f+d35f,max(d32f+d33f+d34f,max(d32f+d33f+d35f,max(d32f+d34f+d35f,max(d33f+d34f+d35f,max(d41f+d42f+d43f,max(d41f+d42f+d44f,max(d41f+d42f+d45f,max(d41f+d43f+d44f,max(d41f+d43f+d45f,max(d41f+d44f+d45f,max(d42f+d43f+d44f,max(d42f+d43f+d45f,max(d42f+d44f+d45f,max(d43f+d44f+d45f,max(d51f+d52f+d53f,max(d51f+d52f+d54f,max(d51f+d52f+d55f,max(d51f+d53f+d54f,max(d51f+d53f+d55f,max(d51f+d54f+d55f,max(d52f+d53f+d54f,max(d52f+d53f+d55f,max(d52f+d54f+d55f,max(d53f+d54f+d55f,max(d61f+d62f+d63f,max(d61f+d62f+d64f,max(d61f+d62f+d65f,max(d61f+d63f+d64f,max(d61f+d63f+d65f,max(d61f+d64f+d65f,max(d62f+d63f+d64f,max(d62f+d63f+d65f,max(d62f+d64f+d65f,max(d63f+d64f+d65f,max(c11f+c12f+c13f,max(c21f+c22f+c23f,max(p11f+p12f+p13f,max(p21f+p22f+p23f))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))");
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
	model.set_global_effort(6, nameEngine);
	model.build_importance_function_adhoc(ifunSpec, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0ul);
	const double confCo(.9);
	const double prec(.3);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(TIMEOUT_(1)));  // estimate for 1 min max
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  == Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
}

SECTION("Steady-state: RESTART, compositional (+ operator), es")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+",
								   fig::PostProcessing(),
	                               0, 42, 0);
	const string nameThr("es");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_global_effort(2, nameEngine);
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0);
	const double confCo(.95);
	const double prec(.2);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(TIMEOUT_(1)));  // estimate for 2 min max
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  == Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
}

SECTION("Steady-state: RESTART, compositional (coarse ifun), hyb")
{
	const string nameEngine("restart");
	const string ifunComp("(Disk11*Disk12*Disk13*Disk14*Disk15*Disk21*Disk22*Disk23*Disk24*Disk25*Disk31*Disk32*Disk33*Disk34*Disk35*Disk41*Disk42*Disk43*Disk44*Disk45*Disk51*Disk52*Disk53*Disk54*Disk55*Disk61*Disk62*Disk63*Disk64*Disk65)+(Controller11*Controller12*Controller13*Controller21*Controller22*Controller23)+(Processor11*Processor12*Processor13*Processor21*Processor22*Processor23)");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto",
								   ifunComp,
								   fig::PostProcessing(fig::PostProcessing::EXP, "exp", 2.0),
	                               3, 1073741952, 1);
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_global_effort(12, nameEngine);
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0);
	const double confCo(.95);
	const double prec(.2);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(TIMEOUT_(2)));  // estimate for 2 min max
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*.4));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
			  == Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
}

SECTION("Steady-state: RESTART, compositional ([+,*] ring), es")
{
	const string nameEngine("restart1");
	const string ifunComp("(Disk11*Disk12*Disk13)+(Disk11*Disk12*Disk14)+(Disk11*Disk12*Disk15)+(Disk11*Disk13*Disk14)+(Disk11*Disk13*Disk15)+(Disk11*Disk14*Disk15)+(Disk12*Disk13*Disk14)+(Disk12*Disk13*Disk15)+(Disk12*Disk14*Disk15)+(Disk13*Disk14*Disk15)+(Disk21*Disk22*Disk23)+(Disk21*Disk22*Disk24)+(Disk21*Disk22*Disk25)+(Disk21*Disk23*Disk24)+(Disk21*Disk23*Disk25)+(Disk21*Disk24*Disk25)+(Disk22*Disk23*Disk24)+(Disk22*Disk23*Disk25)+(Disk22*Disk24*Disk25)+(Disk23*Disk24*Disk25)+(Disk31*Disk32*Disk33)+(Disk31*Disk32*Disk34)+(Disk31*Disk32*Disk35)+(Disk31*Disk33*Disk34)+(Disk31*Disk33*Disk35)+(Disk31*Disk34*Disk35)+(Disk32*Disk33*Disk34)+(Disk32*Disk33*Disk35)+(Disk32*Disk34*Disk35)+(Disk33*Disk34*Disk35)+(Disk41*Disk42*Disk43)+(Disk41*Disk42*Disk44)+(Disk41*Disk42*Disk45)+(Disk41*Disk43*Disk44)+(Disk41*Disk43*Disk45)+(Disk41*Disk44*Disk45)+(Disk42*Disk43*Disk44)+(Disk42*Disk43*Disk45)+(Disk42*Disk44*Disk45)+(Disk43*Disk44*Disk45)+(Disk51*Disk52*Disk53)+(Disk51*Disk52*Disk54)+(Disk51*Disk52*Disk55)+(Disk51*Disk53*Disk54)+(Disk51*Disk53*Disk55)+(Disk51*Disk54*Disk55)+(Disk52*Disk53*Disk54)+(Disk52*Disk53*Disk55)+(Disk52*Disk54*Disk55)+(Disk53*Disk54*Disk55)+(Disk61*Disk62*Disk63)+(Disk61*Disk62*Disk64)+(Disk61*Disk62*Disk65)+(Disk61*Disk63*Disk64)+(Disk61*Disk63*Disk65)+(Disk61*Disk64*Disk65)+(Disk62*Disk63*Disk64)+(Disk62*Disk63*Disk65)+(Disk62*Disk64*Disk65)+(Disk63*Disk64*Disk65)+(Controller11*Controller12*Controller13)+(Controller21*Controller22*Controller23)+(Processor11*Processor12*Processor13)+(Processor21*Processor22*Processor23)");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto",
	                               ifunComp,
	                               fig::PostProcessing(fig::PostProcessing::SHIFT, "shift", 1),
	                               64, 512, 1);
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_global_effort(3, nameEngine);
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name, nameThr, ssPropId);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0);
	const double confCo(.95);
	const double prec(.2);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(TIMEOUT_(3)));
	// Estimate
	model.estimate(ssPropId, *engine, confCrit, ifunSpec);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.4));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
}

} // TEST_CASE [database-R3]

} // namespace tests   // // // // // // // // // // // // // // // // // // //
