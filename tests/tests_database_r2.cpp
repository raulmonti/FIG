//==============================================================================
//
//  tests_database_r2.cpp
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
const string MODEL(tests::models_dir() + "database_r2.sa");

// TAD which will contain the compiled model
fig::ModelSuite& model(fig::ModelSuite::get_instance());

// Steady-state query:
// S((d11f & d12f) | (d11f & d13f) | (d11f & d14f) | (d12f & d13f) | (d12f & d14f) | (d13f & d14f) |
//   (d21f & d22f) | (d21f & d23f) | (d21f & d24f) | (d22f & d23f) | (d22f & d24f) | (d23f & d24f) |
//   (d31f & d32f) | (d31f & d33f) | (d31f & d34f) | (d32f & d33f) | (d32f & d34f) | (d33f & d34f) |
//   (d41f & d42f) | (d41f & d43f) | (d41f & d44f) | (d42f & d43f) | (d42f & d44f) | (d43f & d44f) |
//   (d51f & d52f) | (d51f & d53f) | (d51f & d54f) | (d52f & d53f) | (d52f & d54f) | (d53f & d54f) |
//   (d61f & d62f) | (d61f & d63f) | (d61f & d64f) | (d62f & d63f) | (d62f & d64f) | (d63f & d64f) |
//   (c11f & c12f) | (c21f & c22f) |
//   (p11f & p12f) | (p21f & p22f) )
const double SS_PROB(6.84e-3);  // expected result of steady-state query
int ssPropId(-1);               // index of the query within our TAD

} // namespace   // // // // // // // // // // // // // // // // // // // // //


namespace tests  // // // // // // // // // // // // // // // // // // // // //
{

TEST_CASE("Database with R=2 tests", "[database-R2]")
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
	const string rng("pcg32");
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
	const double confCo(.9);
	const double prec(.2);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(15);  // don't estimate for that long a time
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.2));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) < SS_PROB);
}

SECTION("Estimate steady-state property using RESTART and adhoc ifun")
{
	const string nameEngine("restart");
	const string ifunAdHoc("max(d11f+d12f+d13f+d14f+0,max(d21f+d22f+d23f+d24f+0,max(d31f+d32f+d33f+d34f+0,max(d41f+d42f+d43f+d44f+0,max(d51f+d52f+d53f+d54f+0,max(d61f+d62f+d63f+d64f+0,max(c11f+c12f+0,max(c21f+c22f+0,max(p11f+p12f+0,max(p21f+p22f+0,0))))))))))");
	const fig::ImpFunSpec ifunSpec("algebraic", "adhoc",
	                               ifunAdHoc,
	                               fig::PostProcessing(),
	                               0, 2);
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_splitting(6);
	model.build_importance_function_adhoc(ifunSpec, ssPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, ssPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0ul);
	const double confCo(.9);
	const double prec(.4);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(0);  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
}

SECTION("Estimate steady-state property using RESTART and compositional ifun (op:+)")
{
	const string nameEngine("restart");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto", "+",
	                               fig::PostProcessing(),
	                               0, 32, 0);
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_splitting(8);
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, ssPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0);
	const double confCo(.95);
	const double prec(.2);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(0);  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
}

SECTION("Estimate steady-state property using RESTART and compositional ifun (coarse)")
{
	const string nameEngine("restart");
	const string ifunComp("(Disk11*Disk12*Disk13*Disk14*Disk21*Disk22*Disk23*Disk24*Disk31*Disk32*Disk33*Disk34*Disk41*Disk42*Disk43*Disk44*Disk51*Disk52*Disk53*Disk54*Disk61*Disk62*Disk63*Disk64)+(Controller11*Controller12*Controller21*Controller22)+(Processor11*Processor12*Processor21*Processor22)");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto",
	                               ifunComp,
	                               fig::PostProcessing(fig::PostProcessing::EXP, "exp", 2.0),
	                               3, 16777248, 1);
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_splitting(16);
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, ssPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().front();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0);
	const double confCo(.95);
	const double prec(.2);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(std::chrono::minutes(2));  // estimate for 2 min max
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.8));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*.4));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
}

SECTION("Estimate steady-state property using RESTART and compositional ifun (+,*)")
{
	const string nameEngine("restart");
	const string ifunComp("(Disk11*Disk12)+(Disk11*Disk13)+(Disk11*Disk14)+(Disk12*Disk13)+(Disk12*Disk14)+(Disk13*Disk14)+(Disk21*Disk22)+(Disk21*Disk23)+(Disk21*Disk24)+(Disk22*Disk23)+(Disk22*Disk24)+(Disk23*Disk24)+(Disk31*Disk32)+(Disk31*Disk33)+(Disk31*Disk34)+(Disk32*Disk33)+(Disk32*Disk34)+(Disk33*Disk34)+(Disk41*Disk42)+(Disk41*Disk43)+(Disk41*Disk44)+(Disk42*Disk43)+(Disk42*Disk44)+(Disk43*Disk44)+(Disk51*Disk52)+(Disk51*Disk53)+(Disk51*Disk54)+(Disk52*Disk53)+(Disk52*Disk54)+(Disk53*Disk54)+(Disk61*Disk62)+(Disk61*Disk63)+(Disk61*Disk64)+(Disk62*Disk63)+(Disk62*Disk64)+(Disk63*Disk64)+(Controller11*Controller12)+(Controller21*Controller22)+(Processor11*Processor12)+(Processor21*Processor22)");
	const fig::ImpFunSpec ifunSpec("concrete_split", "auto",
	                               ifunComp,
	                               fig::PostProcessing(fig::PostProcessing::EXP, "exp", 2.0),
	                               40, 160, 1);
	const string nameThr("hyb");
	REQUIRE(model.exists_simulator(nameEngine));
	REQUIRE(model.exists_importance_function(ifunSpec.name));
	REQUIRE(model.exists_importance_strategy(ifunSpec.strategy));
	REQUIRE(model.exists_threshold_technique(nameThr));
	// Prepare engine
	model.set_splitting(11);
	model.build_importance_function_auto(ifunSpec, ssPropId, true);
	model.build_thresholds(nameThr, ifunSpec.name, ssPropId);
	auto engine = model.prepare_simulation_engine(nameEngine, ifunSpec.name);
	REQUIRE(engine->ready());
	// Set estimation criteria
	auto rng = model.available_RNGs().back();
	REQUIRE(model.exists_rng(rng));
	model.set_rng(rng, 0);
	const double confCo(.95);
	const double prec(.2);
	fig::StoppingConditions confCrit;
	confCrit.add_confidence_criterion(confCo, prec);
	model.set_timeout(0);  // unset timeout; estimate for as long as necessary
	// Estimate
	model.estimate(ssPropId, *engine, confCrit);
	auto results = model.get_last_estimates();
	REQUIRE(results.size() == 1ul);
	auto ci = results.front();
	REQUIRE(ci.point_estimate() == Approx(SS_PROB).epsilon(SS_PROB*.4));
	REQUIRE(ci.precision(confCo) > 0.0);
	REQUIRE(ci.precision(confCo) <= Approx(SS_PROB*prec).epsilon(SS_PROB*.2));
	REQUIRE(static_cast<fig::ConfidenceInterval&>(ci).precision()
	          == Approx(SS_PROB*prec).epsilon(SS_PROB*0.1));
}

} // TEST_CASE [database-R2]

} // namespace tests   // // // // // // // // // // // // // // // // // // //
