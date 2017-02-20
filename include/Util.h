/* Leonardo Rodríguez */
//@todo unify with other "utilities" headers.

#ifndef UTIL_H
#define UTIL_H

#include <memory>
#include <vector>
#include <sstream>
#include <iostream>
#include <map>
#include <algorithm>

using std::forward;
using std::unique_ptr;
using std::stringstream;
using std::string;
using std::endl;
using std::vector;
using std::shared_ptr;
using std::map;

template<typename T>
using shared_vector = std::vector<shared_ptr<T>>;

template<typename K, typename T>
using shared_map = std::map<K, shared_ptr<T>>;

template<typename T>
vector<T> concat(vector<T> &v1, const vector<T> &v2) {
    v1.insert(v1.end(), v2.begin(), v2.end());
    return v1;
}

template<typename T>
void print_all(const T &v) {
    for (const auto &x : v) {
        std::cout << " " << x;
    }
    std::cout << std::endl;
}

template<typename T>
shared_vector<T> shared_copy(const vector<T>& vec) {
    shared_vector<T> copy;
    for (auto a : vec) {
        copy.push_back(std::make_shared<T>(a));
    }
    return (copy);
}


template<typename T>
void sort_by_lt(shared_vector<T>& v) {
    auto by_lt = [] (const T& t1, const T& t2) {
        return (t1 < t2);
    };
    std::sort(v.begin(), v.end(), by_lt);
}

//standard in c++14
/// @todo: enclose definition with the apropiate directive
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif
