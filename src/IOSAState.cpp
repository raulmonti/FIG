/* Leonardo Rodríguez */

#include <cassert>
#include <iostream>
#include <sstream>

#include "IOSAState.h"


namespace iosa {

void State::add_variable(const std::string &name, const FixedRange& range) {
    assert(ranges.find(name) == ranges.end());
    ranges.insert(std::make_pair(name,range));
    state_pos_t i = values.size();
    pos[name] = i;
    values.push_back(range.get_min());
}

void State::set_variable_value(const std::string &name, state_value_t value) {
    assert(pos.find(name) != pos.end());
    state_pos_t i = pos.at(name);
    assert(i < values.size());
    values[i] = value;
}

state_value_t State::get_variable_value(const std::string &name) const {
    state_pos_t i = pos.at(name);
    assert(i < values.size());
    return values[i];
}

bool State::is_valid() const {
    bool result = true;
    for (auto entry : pos) {
        state_pos_t i = entry.second;
        state_value_t v = values[i];
        FixedRange range = ranges.at(entry.first);
        if (v < range.get_min() || v > range.get_max()) {
            result = false;
        }
    }
    return (result);
}

void State::print_state(std::ostream& ss) const {
    for (auto entry : pos) {
        const std::string &name = entry.first;
        ss << name << ": " << this->get_variable_value(name) << " ";
    }
}

bool State::operator==(const State &that) const {
    return (this->values == that.values);
}

bool State::operator!=(const State &that) const {
    return (this->values != that.values);
}

bool State::operator<(const State&that) const {
    return (this->values < that.values);
}

} //
