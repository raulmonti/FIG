/* Leonardo Rodríguez */
#include "ExpStateUpdater.h"

namespace fig {

void ExpStateUpdater::prepare(const PositionsMap& posMap) {
    ExpStateEvaluator::prepare(posMap);
    for (const std::string &name : updatesVar) {
        updatesPos[name] = posMap.at(name);
    }
}

void ExpStateUpdater::prepare(const State<STYPE>& state) {
    ExpStateEvaluator::prepare(state);
    for (const std::string &name : updatesVar) {
        updatesPos[name] = state.position_of_var(name);
    }
}

void ExpStateUpdater::update(StateInstance &state) const {
    std::vector<STYPE> results = eval_all(state);
    for (size_t i = 0; i < results.size(); i++) {
        state[updatesPos.at(updatesVar[i])] = results[i];
    }
}

void ExpStateUpdater::update(State<STYPE> &state) const {
    std::vector<STYPE> results = eval_all(state);
    for (size_t i = 0; i < results.size(); i++) {
        state[updatesPos.at(updatesVar[i])]->assign(results[i]);
    }
}

} // namespace fig
