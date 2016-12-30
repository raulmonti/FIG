#include "ExpState.h"

namespace fig {

void ExpNameCollector::visit(shared_ptr<LocExp> node) noexcept {
    shared_ptr<Location> loc = node->get_exp_location();
    const std::string &name = loc->get_identifier();
    if(loc->is_array()) {
        if (arrays.find(name) == arrays.end()) {
            shared_ptr<ArrayDecl> decl = loc->to_array_position()->get_decl();
            assert(decl != nullptr);
            arrays[name] = decl->get_data();
        }
    } else {
        vars.insert(name);
    }
}

void ExpNameCollector::visit(shared_ptr<BinOpExp> node) noexcept {
    node->get_first_argument()->accept(*this);
    node->get_second_argument()->accept(*this);
}

void ExpNameCollector::visit(shared_ptr<UnOpExp> node) noexcept {
    node->get_argument()->accept(*this);
}

template<typename T>
ExpState<T>::ExpState(const std::vector<std::shared_ptr<Exp>> &astVec) {
    std::unordered_set<std::string> vars;
    std::unordered_map<std::string, ArrayData> arrays;
    for (shared_ptr<Exp> ast : astVec) {
        ExpNameCollector visitor (vars, arrays);
        ast->accept(visitor); //fills vars and arrays
    }
    //compute local data size
    int size = vars.size(); //number of variables
    for (auto& pair : arrays) { //plus array sizes
        size += pair.second.data_size;
    }
    //create space
    mem_.resize(size);
    //create simple variables
    size_t i = 0;
    for (const std::string& var : vars) {
        SData sData (i, 0);
        vars_[var] = VarData::build_simple_var(sData);
        mem_[i] = T(0);
        i++;
    }
    i = vars.size();
    //starts to copy arrays at the end of the region for variables
    for (auto& pair : arrays) {
        AData info (i, 0, pair.second.data_size);
        vars_[pair.first] = VarData::build_array_var(info);
        for (int v : pair.second.data_inits) {
            mem_[i] = T(v);
            i++;
        }
    }
}

template<typename T>
void ExpState<T>::project_positions(const State<STATE_INTERNAL_TYPE> &state)
noexcept {
    for (auto &pair : vars_) {
        const std::string &name = pair.first;
        VarData &v = pair.second;
        if (v.type_ == VarType::SIMPLE) {
            v.data_.sData_.externalPos_ = state.position_of_var(name);
        } else if (v.type_ == VarType::ARRAY) {
            v.data_.aData_.fstExternalPos_ = state.position_of_array_fst(name);
            assert(v.data_.aData_.size_ == state.array_size(name));
        }
    }
}

template<typename T>
void ExpState<T>::project_positions(const PositionsMap &posMap)
noexcept {
    for (auto &pair : vars_) {
        const std::string &name = pair.first;
        VarData &v = pair.second;
        if (v.type_ == VarType::SIMPLE) {
            v.data_.sData_.externalPos_ = posMap.at(name);
        } else if (v.type_ == VarType::ARRAY) {
            const std::string &fstStr = name + "[0]";
            v.data_.aData_.fstExternalPos_ = posMap.at(fstStr);
        }
    }
}

template<typename T>
void ExpState<T>::project_values(const State<STATE_INTERNAL_TYPE> &state)
noexcept {
    for (auto &pair : vars_) {
        const std::string &name = pair.first;
        VarData &v = pair.second;
        if (v.type_ == VarType::SIMPLE) {
            pos_t localPos = v.data_.sData_.localPos_;
            mem_[localPos] = state[name]->val();
        } else if (v.type_ == VarType::ARRAY) {
            pos_t localPos = v.data_.aData_.fstLocalPos_;
            pos_t size = v.data_.aData_.size_;
            for (pos_t i = 0; i < size; i++) {
                mem_[localPos + i] = T(state.array_value(name, i));
            }
        }
    }
}

template<typename T>
void ExpState<T>::project_values(const StateInstance &state) noexcept {
    for (auto &pair : vars_) {
        VarData &v = pair.second;
        if (v.type_ == VarType::SIMPLE) {
            pos_t localPos = v.data_.sData_.localPos_;
            pos_t externalPos = v.data_.sData_.externalPos_;
            mem_[localPos] = state[externalPos];
        } else if (v.type_ == VarType::ARRAY) {
            pos_t localPos = v.data_.aData_.fstLocalPos_;
            size_t size = v.data_.aData_.size_;
            pos_t externalPos = v.data_.aData_.fstExternalPos_;
            for (pos_t i = 0; i < size; i++) {
                mem_[localPos + i] = T(state[externalPos + i]);
            }
        }
    }
}

template<typename T>
void ExpState<T>::fill_symbol_table(exprtk::symbol_table<T>& table) noexcept {
    for (auto &pair : vars_) {
        const std::string &name = pair.first;
        VarData &v = pair.second;
        if (v.type_ == VarType::SIMPLE) {
            pos_t localPos = v.data_.sData_.localPos_;
            table.add_variable(name, mem_[localPos]);
        } else if (v.type_ == VarType::ARRAY) {
            pos_t localPos = v.data_.aData_.fstLocalPos_;
            size_t size = v.data_.aData_.size_;
            auto view = exprtk::make_vector_view(mem_, size, localPos);
            table.add_vector(name, view);
        }
    }
}

template class ExpState<float>;

} //namespace fig
