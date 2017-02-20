/* Leonardo Rodríguez */

#ifndef IOSASTATE_H
#define IOSASTATE_H

#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace iosa {

using state_value_t = int;
using state_pos_t = unsigned long int;

/**
 * @brief Range of a variable
 */
class FixedRange {
private:
    state_value_t min;
    state_value_t max;
public:
    FixedRange(state_value_t min, state_value_t max) :
        min {min}, max {max} {}

    state_value_t get_min() const {
        return (min);
    }

    state_value_t get_max() const {
        return (max);
    }
};

/** @brief An explicit state. Represent a vertex
 * in the underlying graph of the explicit automata.
 */
class State {
private:
    /// STATE VECTOR, holds the variable values
    std::vector<state_value_t> values;
    /// VARIABLE -> POSITION ON THE STATE VECTOR
    std::unordered_map<std::string, state_pos_t> pos;
    /// VARIABLE -> RANGE
    std::map<std::string, FixedRange> ranges;

public:
    State() {}
    State(const State& state) = default;

    /// Add a variable in the state
    void add_variable(const std::string &name, const FixedRange& range);

    /// Change the value of a variable
    void set_variable_value(const std::string &name, state_value_t value);

    /// Return the value of a variable
    state_value_t get_variable_value(const std::string &name) const;

    /// Check if the variable value has a value within the allowed range
    bool is_valid() const;

    /// Print debugging info
    void print_state(std::ostream& ss) const;

    /// Compare states by ther values
    bool operator==(const State &that) const;
    bool operator!=(const State &that) const;
    bool operator<(const State &that) const;
};

}

#endif
