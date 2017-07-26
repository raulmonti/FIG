/* Leonardo Rodríguez */
#include "ExpState.h"

namespace fig {

void ExpNameCollector::visit(shared_ptr<LocExp> node) noexcept {
	assert(nullptr != node);
    shared_ptr<Location> loc = node->get_exp_location();
	assert(nullptr != loc->get_decl());
	const std::string &name = loc->get_identifier();
	if(loc->is_array_position() ||
			loc->get_decl()->is_array()) {
        //appears as "array[4]" or as "fsteq(array, 1)"
        if (arrays.find(name) == arrays.end()) {
            shared_ptr<ArrayDecl> decl = loc->get_decl()->to_array();
            assert(decl != nullptr);
            arrays[name] = decl->get_data();
        }
    } else {
        //add simple variables only
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
ArrayFunctions::FstEqFunction<T> ExpState<T>::fsteq_;
template<typename T>
ArrayFunctions::LstEqFunction<T> ExpState<T>::lsteq_;
template<typename T>
ArrayFunctions::RndEqFunction<T> ExpState<T>::rndeq_;
template<typename T>
ArrayFunctions::MaxFromFunction<T> ExpState<T>::maxfrom_;
template<typename T>
ArrayFunctions::MinFromFunction<T> ExpState<T>::minfrom_;
template<typename T>
ArrayFunctions::SumFromFunction<T> ExpState<T>::sumfrom_;
template<typename T>
ArrayFunctions::ConsecFunction<T> ExpState<T>::consec_;
template<typename T>
ArrayFunctions::BrokenFunction<T> ExpState<T>::broken_;
template<typename T>
ArrayFunctions::FstExcludeFunction<T> ExpState<T>::fstexclude_;

template<typename T>
ExpState<T>::ExpState(const std::vector<std::shared_ptr<Exp>> &astVec) {
    //::run_example_and_exit();
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
        assert(i < (size_t) size);
        mem_[i] = T(0);
        i++;
    }
    i = vars.size();
    //starts to copy arrays at the end of the region for variables
    for (auto& pair : arrays) {
        AData info (i, 0, pair.second.data_size);
        vars_[pair.first] = VarData::build_array_var(info);
        for (int v : pair.second.data_inits) {
            assert(i < (size_t) size);
            mem_[i] = T(v);
            i++;
        }
    }
    fill_symbol_table();
}

template<typename T>
ExpState<T>::ExpState(const ExpState& that) {
    new (&mem_) std::vector<T>(that.mem_);
    assert(mem_.size() == that.mem_.size());
    assert(mem_ == that.mem_);
    new (&vars_) std::unordered_map<std::string, VarData>(that.vars_);
    //for (auto& p : vars_) {
    //    assert(p.second == that.vars_.at(p.first));
    //}
    new (&table_) exprtk::symbol_table<T>();
    fill_symbol_table();
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
            assert(localPos < mem_.size());
            mem_[localPos] = state[name]->val();
        } else if (v.type_ == VarType::ARRAY) {
            pos_t localPos = v.data_.aData_.fstLocalPos_;
            pos_t size = v.data_.aData_.size_;
            for (pos_t i = 0; i < size; i++) {
                assert(localPos + i < mem_.size());
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
            assert(localPos < mem_.size());
            mem_[localPos] = state[externalPos];
        } else if (v.type_ == VarType::ARRAY) {
            pos_t localPos = v.data_.aData_.fstLocalPos_;
            size_t size = v.data_.aData_.size_;
            pos_t externalPos = v.data_.aData_.fstExternalPos_;
            for (pos_t i = 0; i < size; i++) {
                assert(localPos + i < mem_.size());
                mem_[localPos + i] = T(state[externalPos + i]);
            }
        }
    }
}

template<typename T>
void ExpState<T>::fill_symbol_table() noexcept {
    for (auto &pair : vars_) {
        const std::string &name = pair.first;
        VarData &v = pair.second;
        if (v.type_ == VarType::SIMPLE) {
            pos_t localPos = v.data_.sData_.localPos_;
            assert(localPos < mem_.size());
            table_.add_variable(name, mem_[localPos]);
        } else if (v.type_ == VarType::ARRAY) {
            pos_t fstPos = v.data_.aData_.fstLocalPos_;
            size_t size = v.data_.aData_.size_;
            //do not use vector_view with automatic storage:
            // auto view = exprtk::make_vector_view(mem_, size, fsstPos);
            table_.add_vector(name, ((T*) mem_.data()) + fstPos, size);
        }
    }
    add_functions();
}

template<typename T>
void ExpState<T>::add_functions() noexcept {
    table_.add_function("fsteq", fsteq_);
    table_.add_function("lsteq", lsteq_);
    table_.add_function("rndeq", rndeq_);
    table_.add_function("maxfrom", maxfrom_);
    table_.add_function("minfrom", minfrom_);
    table_.add_function("sumfrom", sumfrom_);
    table_.add_function("consec", consec_);
    table_.add_function("broken", broken_);
    table_.add_function("fstexclude", fstexclude_);
}


template<typename T>
void ExpState<T>::print_table() const noexcept {
    std::vector<std::string> v;
    std::cout << "Print memory:";
    for (const T &v : mem_) {
        std::cout << v << ", ";
    }
    std::cout << std::endl;
    table_.get_vector_list(v);
    if (v.empty()) {
        std::cout << "Symbol table has no vectors" << std::endl;
    }
    std::vector<std::pair<std::string, T>> vars;
    table_.get_variable_list(vars);
    if (vars.empty()) {
        std::cout << "Symbol table has no simple variables" << std::endl;
    }
    for (std::string name : v) {
        exprtk::details::vector_holder<T> *vh = table_.get_vector(name);
        std::cout << "vector " << name << ": size "
                  << vh->size() << ", values: ";
        for (size_t i = 0; i < vh->size(); i++) {
            std::cout << (vh->data())[i] << ", ";
        }
        std::cout << std::endl;
    }
    for (auto&p : vars) {
        std::cout << "variable " << p.first << ": " << p.second << std::endl;
    }
}

template class ExpState<float>;
template class ExpState<double>;


} //namespace fig

/*
namespace {
// Improvised unit test.
void run_example_and_exit() {
    std::vector<float> vec
            = {0,1,0,1,1,1,0,1,0,1,0,0}; //change it!
    exprtk::symbol_table<float> table;
    exprtk::parser<float> parser;
    ArrayFunctions::FstEqFunction<float> fsteq;
    ArrayFunctions::LstEqFunction<float> lsteq;
    ArrayFunctions::RndEqFunction<float> rndeq;
    ArrayFunctions::MinFromFunction<float> minfrom;
    ArrayFunctions::MaxFromFunction<float> maxfrom;
    ArrayFunctions::SumFromFunction<float> sumfrom;
    ArrayFunctions::ConsecFunction<float> consec;
    ArrayFunctions::BrokenFunction<float> broken;
    table.add_function("fsteq", fsteq);
    table.add_function("lsteq", lsteq);
    table.add_function("rndeq", rndeq);
    table.add_function("minfrom", minfrom);
    table.add_function("maxfrom", maxfrom);
    table.add_function("sumfrom", sumfrom);
    table.add_function("consec", consec);
    table.add_function("broken", broken);
    table.add_vector("array", vec);
    std::vector<std::string> tests = {
        "fsteq(array, 4)",
        "lsteq(array, 3)",
        "rndeq(array, 3)",
        "minfrom(array, 1)",
        "minfrom(array, 5)",
        "maxfrom(array, 0)",
        "maxfrom(array, 7)",
        "sumfrom(array, 0)",
        "sumfrom(array, 5)",
        "broken(array, 0)",
        "consec(array, 2)",
        "consec(array, 3)",
        "consec(array, 4)",
    };
    std::cout << "Testing vector: ";
    for (float& x : vec) {
        std::cout << x << ", ";
    }
    std::cout << std::endl;
    for (std::string t : tests) {
        exprtk::expression<float> expr;
        expr.register_symbol_table(table);
        parser.compile(t, expr);
        std::cout << t << " = " << expr.value() << std::endl;
    }
    std::cout << "Modified vector: ";
    for (float& x : vec) {
        std::cout << x << ", ";
    }
    std::cout << std::endl;
    throw_FigException("Bye!");
}
}
*/


