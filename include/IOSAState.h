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

class State {
private:
    std::vector<state_value_t> values;
    std::unordered_map<std::string, state_pos_t> pos;
    std::map<std::string, FixedRange> ranges;

public:
    State() {}

    State(const State& state) = default;

    void add_variable(const std::string &name, const FixedRange& range);
    void set_variable_value(const std::string &name, state_value_t value);
    state_value_t get_variable_value(const std::string &name) const;
    bool is_valid() const;
    void print_state(std::ostream& ss) const;
    bool operator==(const State &that) const;
    bool operator!=(const State &that) const;
    bool operator<(const State &that) const;
};

}

#endif
