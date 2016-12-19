/* Leonardo Rodríguez */
#include "ExpStateUpdater.h"


namespace fig {

void ExpStateUpdater::prepare(const PositionsMap& posMap) {
    ExpStateEvaluator::prepare(posMap);
    for (size_t i = 0; i < updatesVar.size(); i++) {
        updatesPos[i] = posMap.at(updatesVar[i]);
    }
}

void ExpStateUpdater::prepare(const State<STYPE>& state) {
    ExpStateEvaluator::prepare(state);
    for (size_t i = 0; i < updatesVar.size(); i++) {
        updatesPos[i] = state.position_of_var(updatesVar[i]);
    }
}

void ExpStateUpdater::update(StateInstance &state) const {
    std::vector<STYPE> results = eval_all(state);
    for (size_t i = 0; i < results.size(); i++) {
        state[updatesPos[i]] = results[i];
    }
}

void ExpStateUpdater::update(State<STYPE> &state) const {
    std::vector<STYPE> results = eval_all(state);
    for (size_t i = 0; i < results.size(); i++) {
        state[updatesPos[i]]->assign(results[i]);
    }
    // std::cout << results.size() << " - UPDATE: " << std::endl;
    // state.print_out(std::cout, true);
}

} // namespace fig
