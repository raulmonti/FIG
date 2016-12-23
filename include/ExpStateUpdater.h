/* Leonardo Rodríguez */

#ifndef EXP_STATE_UPDATER_H
#define EXP_STATE_UPDATER_H

#include <ExpStateEvaluator.h>

namespace fig {

class ExpStateUpdater : public ExpStateEvaluator {
private:
    NameContainer updatesVar;
    std::vector<size_t> updatesPos;

public:
    ExpStateUpdater(const NameContainer& updates, const ExpContainer &expVec) :
        ExpStateEvaluator(expVec), updatesVar {updates} {
        assert(updates.size() == expVec.size());
        updatesPos.resize(updates.size());
    }

    void prepare(const PositionsMap& posMap) noexcept override;
    void prepare(const State<STYPE>& state) noexcept override;
    void update(StateInstance& state) const noexcept;
    void update(State<STYPE>& state) const noexcept;
};

}
#endif
