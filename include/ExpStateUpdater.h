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

    void prepare(const PositionsMap& posMap) override;
    void prepare(const State<STYPE>& state) override;
    void update(StateInstance& state) const;
    void update(State<STYPE>& state) const;
};

}
#endif
