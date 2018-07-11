/* Leonardo Rodríguez */

#ifndef _EXP_STATE_H_
#define _EXP_STATE_H_

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <tuple>

#include <ModelAST.h>
#include <State.h>
#include <FigException.h>
#include <ArrayFunctions.h>

namespace fig {

typedef size_t pos_t;
using std::shared_ptr;

/// @brief Collects every variable name occurring on the AST into a vector
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

/** An internal state used to evaluate expressions during simulation.
 *  Uses Exprtk library to archive efficient evaluation.
 *
 *  @example
 *   An abstract state {x -> 2, y -> 5, arr -> [9,5,6], z -> 1} is represented as
 *
 *   VALUES    [2][5][9][5][6][1]
 *   POSITIONS  0  1  2  3  4  5
 *
 *   VARIABLE-POSITION MAP
 *   "x" -> [ 0, ix ]
 *   "y" -> [ 1, iy ]
 *   "arr" -> [2, iarr, 3]
 *   "z" -> [5, iz]
 *
 *   "ix", "iy", "iarr", "iz" are external positions used to update the
 *    state according to the main simulation state.
 *    e.g to update "x" we assign VALUES[0] := MainState[ix]
 *        to update "arr" we assign VALUES[2 + 0] = MainState[iarr + 0]
 *                                  VALUES[2 + 1] = MainState[iarr + 1]
 *                                  VALUES[2 + 2] = MainState[iarr + 2]
 *    MainState is actually a \ref fig::State or \ref fig::StateInstance object
 *
 *
 * @todo I think that *this* should be the MainState to avoid the "projection"
 * overhead. We could keep a fixed global symbol table to evaluate all our
 * expressions without the need of any projection.
 */
template<typename T>
class ExpState {
private:
    /// Functions over arrays.
    static ArrayFunctions::FstEqFunction<T> fsteq_;
    static ArrayFunctions::LstEqFunction<T> lsteq_;
    static ArrayFunctions::RndEqFunction<T> rndeq_;
    static ArrayFunctions::MaxFromFunction<T> maxfrom_;
    static ArrayFunctions::MinFromFunction<T> minfrom_;
    static ArrayFunctions::SumFromFunction<T> sumfrom_;
	static ArrayFunctions::SumMaxFunction<T> summax_;
	static ArrayFunctions::ConsecFunction<T> consec_;
    static ArrayFunctions::BrokenFunction<T> broken_;
    static ArrayFunctions::FstExcludeFunction<T> fstexclude_;

    /// Each entry on the VECTOR-POSITION map is a variable or an array:
    enum class VarType {
        SIMPLE, ARRAY
    };

    /// Simple variable entry e.g "x" -> [0, ix]
    ///@see example above.
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

    /// Array variable entry e.g "arr" -> [2, iarr, 3]
    /// @see example above
    struct AData {
        pos_t fstLocalPos_; //local position of the first element
        pos_t fstExternalPos_; //external position of the first element
        size_t size_; //size of the array

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

    /// An entry can be "simple" or "array"
    struct VarData {
        /// Tag to distinguish which kind of entry we have
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

    /// The vector of values of the state.
    std::vector<T> mem_;
    /// Variable-Positions Map
    std::unordered_map<std::string, VarData> vars_;
    /// Symbol table used to build exprtk-expressions
    exprtk::symbol_table<T> table_;

public:
    ExpState(const std::vector<std::shared_ptr<Exp>>& astVec);
    ExpState(const ExpState& that);
	ExpState(ExpState &&that);
    ExpState& operator=(const ExpState& state) = delete;

    /// Associate an external position to each variable on the state.
    void project_positions(const State<STATE_INTERNAL_TYPE> &state) noexcept;

    /// Associate an external position to each variable on the state.
    void project_positions(const PositionsMap &posMap) noexcept;

    /// Update variable values according to the positions projected by
    /// \ref project_positions.
    /// @note project_positions must be called first.
    void project_values(const State<STATE_INTERNAL_TYPE> &state) noexcept;
    void project_values(const StateInstance &state) noexcept;

    /// Associate our internal symbol table to a given expression
    void register_expression(exprtk::expression<T> &e) {
        e.register_symbol_table(table_);
    }

    /// Print debug info
    void print_table() const noexcept;

private:

    /// Register every symbol in the table.
    void fill_symbol_table() noexcept;

    /// Add functions to the table.
    void add_functions() noexcept;

};

} //namespace fig


#endif
