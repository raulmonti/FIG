#include <list>
#include <string>

using std::string;

class ImportanceFunction;
class SimulationEngine;

class ModelSuite
{
	static ModuleNetwork&       model;
	static std::list<Property>  properties;
	static StoppingCondition    goal;
	static std::map< string, ImportanceFunction* >  iFuns;
	static std::map< string, SimulationEngine& >    simulators;

public:

	void process_batch(std::list<string> importanceStrategies,
	                   std::list<string> simulationStrategies)
	{
		// For each property
		for (auto prop: properties) {
			// For each importance strategy (null, auto, ad hoc...)
			for (auto impStrat: importanceStrategies) {
				// FIXME: notice impFun assessment requires the Property
				auto impFun = iFuns[impStrat]->assess(model, prop);
				set_current_ifun(impFun);  // access by "Event inspect(Traial)"
				// For each simulation strategy (nosplit, restart...)
				for (auto simStrat: simulationStrategies) {
					auto engine = simulators[simStrat];
					if (!engine.is_compatible_importance(impStrat) ||
						!engine.is_compatible_property(prop->type))
						continue;
					if (goal.is_value())
						for (auto confCrit: goal.confidence_criteria())
							estimate_value(prop, impFun, engine, confCrit);
					else
						for (auto budget: goal.budgets())
							estimate_budget(prop, impFun, engine, budget);
				}
				impFun.release_resources();
			}
		}
	}
	
	void estimate_value(const Property&           prop,
	                    const ImportanceFunction& impFun,
	                    const SimulationEngine&   engine,
	                    const ConfidenceCriteria& confCrit)
	{
		ConfidenceInterval ci(confCrit);
		size_t  numRuns = min_batch_size();
		float startTime = omp_get_wtime();
		do {
			SimulationResult estimate = engine.simulate(prop, impFun, numRuns);
			if (estimate.is_invalid)
				numRuns *= 2;
			else
				ci.update(estimate.value);
		} while (!ci.satisfied_criterion());
		log_estimation(ci, omp_get_wtime()-startTime, engine, impFun);
	}
	
	void estimate_budget(const Property&           prop,
	                     const ImportanceFunction& impFun,
	                     const SimulationEngine&   engine,
	                     const TimeBudget&         budget)
	{
		ConfidenceInterval ci;
		long timeBudget = budget.seconds();
		auto timeout = [&](){ log_estimation(ci, timeBudget, engine, impFun); };
		signal(SIGALRM, &timeout);  // #include <csignal>
		alarm(timeBudget);          // #include <unistd.h>
		engine.simulate(prop, impFun, ci);
	}
}
