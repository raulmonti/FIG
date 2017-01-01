#ifndef _EXP_STATE_H_
#define _EXP_STATE_H_

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <tuple>

#include <ModelAST.h>
#include <State.h>
#include <FigException.h>
#include "exprtk.hpp"

namespace fig {

typedef size_t pos_t;
using std::shared_ptr;

/// @brief Collects every variable name occuring on the AST into a vector
class ExpNameCollector: public Visitor {
    std::unordered_set<std::string> &vars;
    std::unordered_map<std::string, ArrayData>& arrays;
public:
    ExpNameCollector(std::unordered_set<std::string> &vars,
                     std::unordered_map<std::string, ArrayData> &arrays)
        : vars {vars}, arrays {arrays} {}

    void visit(shared_ptr<LocExp> node) noexcept override;
    void visit(shared_ptr<BinOpExp> node) noexcept override;
    void visit(shared_ptr<UnOpExp> node) noexcept override;
};

template<typename T>
class ExpState {
private:

    enum class VarType {
        SIMPLE, ARRAY
    };

    struct SData {
        pos_t localPos_;
        pos_t externalPos_;

        SData() {
            localPos_ = 0;
            externalPos_ = 0;
        }

        SData(pos_t local, pos_t external) :
            localPos_ {local}, externalPos_ {external}
        {}

        bool operator==(const SData& sdata) const {
            return (this->localPos_ == sdata.localPos_) &&
                    (this->externalPos_ == sdata.externalPos_);
        }
    };

    struct AData {
        pos_t fstLocalPos_;
        pos_t fstExternalPos_;
        size_t size_;

        AData() {
            fstLocalPos_ = 0;
            fstExternalPos_ = 0;
            size_ = 0;
        }

        AData(pos_t fstLocal, pos_t fstExternal, size_t size) :
            fstLocalPos_ {fstLocal}, fstExternalPos_ {fstExternal}, size_ {size}
        {}

        bool operator==(const AData &that) const {
            return (this->fstExternalPos_ == that.fstExternalPos_) &&
                   (this->fstLocalPos_ == that.fstLocalPos_) &&
                   (this->size_ == that.size_);
        }

    };

    struct VarData {
        VarType type_;
        struct data__ { //wanted a union but had problems.
            SData sData_;
            AData aData_;
        } data_;

        static VarData build_simple_var(SData data) {
            VarData d;
            d.type_ = VarType::SIMPLE;
            d.data_.sData_ = data;
            return (d);
        }

        static VarData build_array_var(AData data) {
            VarData d;
            d.type_ = VarType::ARRAY;
            d.data_.aData_ = data;
            return (d);
        }

        bool operator==(const VarData& that) const {
            if (this->type_ != that.type_) {
                return (false);
            }
            if (this->type_ == VarType::SIMPLE) {
                return (this->data_.sData_ == that.data_.sData_);
            }
            if (this->type_ == VarType::ARRAY) {
                return (this->data_.aData_ == that.data_.aData_);
            }
            return (false);
        }

    };

    std::vector<T> mem_;
    std::unordered_map<std::string, VarData> vars_;
    exprtk::symbol_table<T> table_;

public:
    ExpState(const std::vector<std::shared_ptr<Exp>>& astVec);
    ExpState(const ExpState& that);
    ExpState(ExpState &&that) = delete;
    ExpState& operator=(const ExpState& state) = delete;


    void project_positions(const State<STATE_INTERNAL_TYPE> &state) noexcept;
    void project_positions(const PositionsMap &posMap) noexcept;
    void project_values(const State<STATE_INTERNAL_TYPE> &state) noexcept;
    void project_values(const StateInstance &state) noexcept;

    void register_expression(exprtk::expression<T> &e) {
        e.register_symbol_table(table_);
    }

    void print_table() const noexcept;

private:
    void fill_symbol_table() noexcept;

};

} //namespace fig


#endif
