/* Leonardo Rodríguez */

#include "ExpStateUpdater.h"


namespace fig {

ExpContainer
ExpStateUpdater::append_arrays_indices(const ExpContainer &exps,
                                       const LocationContainer &updates) {
    num_arr_pos = 0;
    ExpContainer astVec;
    for (shared_ptr<Exp> exp : exps) {
        astVec.push_back(exp);
    }
    for (shared_ptr<Location> loc : updates) {
        if (loc->is_array_position()) {
            astVec.push_back(loc->to_array_position()->get_index());
            num_arr_pos++;
        }
    }
    return (astVec);
}

ExpStateUpdater::ExpStateUpdater(const LocationContainer &updates,
                                 const ExpContainer &expVec)
    : evaluator_(append_arrays_indices(expVec, updates)) {
    assert(expVec.size() == updates.size());
    num_updates = expVec.size();
    //create space for the total number of updates
    result_accs_.resize(num_updates);
    //num_updates holds the first position of the evaluator's internal vector
    //that contains an array index to be evaluated ("N" in the note above)
    size_t first_index = num_updates;
    size_t offset = 0;
    for (size_t i = 0; i < num_updates; i++) {
        //append_array_indices must iterate in the same order
        shared_ptr<Location> loc = updates[i];
        const std::string &name = loc->get_identifier();
        if (loc->is_array_position()) {
            shared_ptr<ArrayDecl> decl
                    = loc->get_decl()->to_array();
            assert(decl != nullptr);
            size_t size = decl->get_data().data_size;
            // this array has its index in the N + offset position
            // of evaluator_.astVec
            pos_t pos = first_index + offset;
            auto acc = ResultAcceptor::build_array_acc(name, 0, pos, size);
            result_accs_[i] = acc;
            offset++;
        } else {
            result_accs_[i] = ResultAcceptor::build_simple_acc(name, 0);
        }
    }
}

void ExpStateUpdater::prepare(const State<STYPE>& state) noexcept {
    evaluator_.prepare(state);
    for (ResultAcceptor& acc : result_accs_) {
        if (acc.tag_ == Tag::ARRAY) {
            const std::string& name = acc.array_acc_.name_;
            acc.array_acc_.fstExternalPos_ = state.position_of_array_fst(name);
            assert(acc.array_acc_.size_ == state.array_size(name));
        } else if (acc.tag_ == Tag::SIMPLE) {
            const std::string& name = acc.var_acc_.name_;
            acc.var_acc_.externalPos_ = state.position_of_var(name);
        }
    }
}

void ExpStateUpdater::prepare(const PositionsMap& posMap) noexcept {
    evaluator_.prepare(posMap);
    for (ResultAcceptor& acc : result_accs_) {
        if (acc.tag_ == Tag::ARRAY) {
            const std::string& name = acc.array_acc_.name_;
            const std::string fstStr = name + "[0]";
            acc.array_acc_.fstExternalPos_ = posMap.at(fstStr);
        } else if (acc.tag_ == Tag::SIMPLE) {
            const std::string& name = acc.var_acc_.name_;
            acc.var_acc_.externalPos_ = posMap.at(name);
        }
    }
}

void ExpStateUpdater::update(State<STYPE> &state) const noexcept {
    std::vector<STYPE> results = evaluator_.eval_all(state);
    assert(results.size() == num_updates + num_arr_pos);
    for (size_t i = 0; i < num_updates; i++) {
        const ResultAcceptor &acc = result_accs_.at(i);
        if (acc.tag_ == Tag::ARRAY) {
            const std::string &name = acc.array_acc_.name_;
            pos_t resultIndex = acc.array_acc_.indexExprPos_;
            pos_t index = results[resultIndex];
            state.update_array(name, index, results[i]);
        } else if (acc.tag_ == Tag::SIMPLE) {
           const std::string &name = acc.var_acc_.name_;
           state[name]->assign(results[i]);
        }
    }
}

void ExpStateUpdater::update(StateInstance &state) const noexcept {
    std::vector<STYPE> results = evaluator_.eval_all(state);
    assert(results.size() == num_updates + num_arr_pos);
    for (size_t i = 0; i < num_updates; i++) {
        const ResultAcceptor &acc = result_accs_.at(i);
        if (acc.tag_ == Tag::ARRAY) {
            pos_t resultIndex = acc.array_acc_.indexExprPos_;
            pos_t index = results[resultIndex];
            pos_t fstPos = acc.array_acc_.fstExternalPos_;
            state[fstPos + index] = results[i];
        } else if (acc.tag_ == Tag::SIMPLE) {
            state[acc.var_acc_.externalPos_] = results[i];
        }
    }
}

} // namespace fig
