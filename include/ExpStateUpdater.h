/* Leonardo Rodríguez */

#ifndef EXP_STATE_UPDATER_H
#define EXP_STATE_UPDATER_H

#include <ExpStateEvaluator.h>

namespace fig {

using LocationContainer = std::vector<shared_ptr<Location>>;

class ExpStateUpdater {
private:

    struct VarAcceptor {
        std::string name_;
        pos_t externalPos_;
        VarAcceptor() {}
        VarAcceptor(const std::string &name, pos_t externalPos) :
            name_ {name}, externalPos_ {externalPos} {}
    };

    struct ArrayAcceptor {
        std::string name_;
        pos_t fstExternalPos_;
        pos_t indexExprPos_;
        size_t size_;
        ArrayAcceptor() {}
        ArrayAcceptor(const std::string &name,
                      pos_t fstExternalPos,
                      pos_t indexExprPos,
                      size_t size) :
            name_ {name}, fstExternalPos_ {fstExternalPos},
            indexExprPos_ {indexExprPos}, size_ {size} {}
    };

    enum class Tag {SIMPLE, ARRAY};

    struct ResultAcceptor {
        Tag tag_;
        union { //if strange things happen just remove the union.
            VarAcceptor var_acc_;
            ArrayAcceptor array_acc_;
        };

        //anonymous unions deletes default constructors.
        ResultAcceptor() : tag_ {Tag::SIMPLE} {
            new (&var_acc_) VarAcceptor();
        }

        ResultAcceptor(const ResultAcceptor &that) : tag_ {that.tag_} {
            if (tag_ == Tag::SIMPLE) {
                new (&var_acc_) VarAcceptor(that.var_acc_);
            } else if (tag_ == Tag::ARRAY) {
                new (&array_acc_) ArrayAcceptor(that.array_acc_);
            }
        }

        ResultAcceptor& operator=(const ResultAcceptor &that) {
            //destroy myself first.
            if (tag_ == Tag::SIMPLE) {
                var_acc_.~VarAcceptor();
            } else if (tag_ == Tag::ARRAY) {
                array_acc_.~ArrayAcceptor();
            }
            //now I'm ready to copy that.
            tag_ = that.tag_;
            if (tag_ == Tag::SIMPLE) {
                var_acc_ = that.var_acc_;
            } else if (tag_ == Tag::ARRAY) {
                array_acc_ = that.array_acc_;
            }
            return *this;
        }

        ~ResultAcceptor() {
            if (tag_ == Tag::SIMPLE) {
                var_acc_.~VarAcceptor();
            } else if (tag_ == Tag::ARRAY) {
                array_acc_.~ArrayAcceptor();
            }
            tag_.~Tag();
        }

        static inline ResultAcceptor
        build_simple_acc(const std::string &name, pos_t externalPos) {
            ResultAcceptor acc;
            acc.tag_ = Tag::SIMPLE;
            VarAcceptor var_acc (name, externalPos);
            acc.var_acc_ = var_acc;
            return (acc);
        }

        static ResultAcceptor
        build_array_acc(const std::string &name,
                        pos_t fstExternalPos,
                        pos_t indexExprPos,
                        pos_t size) {
            ResultAcceptor acc;
            acc.tag_ = Tag::ARRAY;
            ArrayAcceptor array_acc (name, fstExternalPos, indexExprPos, size);
            acc.array_acc_ = array_acc;
            return (acc);
        }
    };

    /// Evaluator of expressions useful to evaluate both
    /// the update expressions and the array indices.
    // Note: evaluator_.astVec_ has the form [e1, ...., eN] ++ [i1, ..., iM]
    // e1, ..., eN are the expressions to be evaluated in order to know the new
    // values of the locations.
    // i1, ..., iM are the expressions corrresponding to the indices of the
    // arrays positions that should be updated.
    ExpStateEvaluator evaluator_;

    /// Number of array positions to be updated
    // The M above
    size_t num_arr_pos;

    /// Total numbers of updates to be done (arrays positions and vars)
    // The N above
    size_t num_updates;

    /// Vector of size \ref num_updates that holds information about
    /// where to store the results of the evaluation
    std::vector<ResultAcceptor> result_accs_;

    ExpContainer append_arrays_indices(const ExpContainer &exps,
                                       const LocationContainer &updates);

public:
    ExpStateUpdater(const LocationContainer &updates,
                    const ExpContainer &expVec);

    void prepare(const State<STYPE>& state) noexcept;
    void prepare(const PositionsMap& posMap) noexcept;
    void update(State<STYPE>& state) const noexcept;
    void update(StateInstance& state) const noexcept;
};

}
#endif
