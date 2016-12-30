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
    };

    struct VarData {
        VarType type_;
        union data__ {
            SData sData_;
            AData aData_;
            data__(){}
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
    };

    std::vector<T> mem_;
    std::unordered_map<std::string, VarData> vars_;

public:
    ExpState(const std::vector<std::shared_ptr<Exp>>& astVec);
    void project_positions(const State<STATE_INTERNAL_TYPE> &state) noexcept;
    void project_positions(const PositionsMap &posMap) noexcept;
    void project_values(const State<STATE_INTERNAL_TYPE> &state) noexcept;
    void project_values(const StateInstance &state) noexcept;
    void fill_symbol_table(exprtk::symbol_table<T> &table) noexcept;
};

} //namespace fig


#endif
