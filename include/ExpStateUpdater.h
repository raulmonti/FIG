/* Leonardo Rodríguez */

#ifndef EXP_STATE_UPDATER_H
#define EXP_STATE_UPDATER_H

#include <string>
#include <ExpStateEvaluator.h>

namespace fig {

using LocationContainer = std::vector<shared_ptr<Location>>;

/**
 * @brief Evaluate a vector of expressions and save the results
 * on the given locations.
 *
 * @see example on \ref ExpState
 * @example To execute the following sequence of assignments
 *
 *   x' =  x + 4, y' = y + z, arr[1 + x]' = x + y * z
 *
 * we use this class in the following way.
 *
 * Expressions to evaluate by \ref ExpStateEvaluator are:
 *   x + 4, y + z, x + y * z, 1 + x
 *
 * Note that this sequence of expressions has the form
 *    [rhs of each assignment] ++ [index for each lhs array position]
 *
 * When \ref update is called, \ref ExpStateEvaluator builds a vector
 * of "results" that is used to update the state as follows:
 *
 *  x := results[0]
 *  y := results[1]
 *  arr[ results[3] ] := results[2]
 *
 */
class ExpStateUpdater {
private:

    /// We build a table that describes the "place" that will receive
    /// (or accept) the result of each evaluation. That place could
    /// be a simple variable, in that case we store the name of the
    /// variable and its position on the global simulation state...
    struct VarAcceptor {
        std::string name_;
        pos_t externalPos_;
        VarAcceptor() {}
        VarAcceptor(const std::string &name, pos_t externalPos) :
            name_ {name}, externalPos_ {externalPos} {}
    };

    /// ...Or it could be an array.
    struct ArrayAcceptor {
        /// @example "arr" in the example above
        std::string name_;
        /// Position of the first element of the array corresponding to
        /// the global simulation state.
        pos_t fstExternalPos_;
        /// @example "3" in the example above, this will remind us that
        /// "results[3]" has the index in which the array should be updated.
        pos_t indexExprPos_;
        /// size of the array
        size_t size_;
        ArrayAcceptor() {}
        ArrayAcceptor(const std::string &name,
                      pos_t fstExternalPos,
                      pos_t indexExprPos,
                      size_t size) :
            name_ {name}, fstExternalPos_ {fstExternalPos},
            indexExprPos_ {indexExprPos}, size_ {size} {}
    };

    /// A tag to distinguish the acceptor
    enum class Tag {SIMPLE, ARRAY};

    /// The table itself.
    struct ResultAcceptor {
        Tag tag_;
        union { //if strange things happen just remove the union.
            VarAcceptor var_acc_;
            ArrayAcceptor array_acc_;
        };

        //anonymous unions deletes default constructors. Improvise one.
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
            //because maybe I'm simple and 'that' is array, or viceversa.
            if (tag_ == Tag::SIMPLE) {
                var_acc_.~VarAcceptor();
            } else if (tag_ == Tag::ARRAY) {
                array_acc_.~ArrayAcceptor();
            }
            //now I'm ready to copy 'that'.
            tag_ = that.tag_;
            if (tag_ == Tag::SIMPLE) {
                var_acc_ = that.var_acc_;
            } else if (tag_ == Tag::ARRAY) {
                array_acc_ = that.array_acc_;
            }
            return *this;
        }

        ~ResultAcceptor() {
            //http://i0.kym-cdn.com/entries/icons/original/000/006/026/futuramafry.jpg
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
    // i1, ..., iM are the expressions corresponding to the indices of the
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

	ExpStateUpdater(LocationContainer&& updates,
	                ExpContainer&& expVec);

    /// @see ExpState::prepare
    /// Compute the "external position" associtated with each acceptor.
    void prepare(const State<STYPE>& state) noexcept;
    void prepare(const PositionsMap& posMap) noexcept;

    /// First update our internal state, then evaluate our expressions and
    /// finally update the given external state with the computed results.
    /// @note Our "result_accs_" table stores "where" the given state should
    /// be updated.
    /// @note prepare shold be called first
    void update(State<STYPE>& state) const ;
    void update(StateInstance& state) const ;

	/// @copydoc ExprStateEvaluator::to_string()
	inline const std::vector<std::string>& to_string() const noexcept
		{ return evaluator_.to_string(); }
};

}
#endif
