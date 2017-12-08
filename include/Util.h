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


/// For point x return f(x), where \a "f : R -> R" is the linear function
/// such that \a "f(x0) = y0" and \a "f(x1) = y1"
/// @param  x  Point whose value will be evaluated in the linear function
template< typename T >
T linear_interpol(const T& x0, const T& x1,
                  const T& y0, const T& y1,
                  const T& x)
{
	static_assert(std::is_floating_point<T>::value,
	              "ERROR: type mismatch. Only floating point numbers supported");
	return (y1 / (x1 - x0) -
	        y0 / (x1 - x0)) * x
	        + ((x1 * y0)/(x1 - x0))
	        - ((x0 * y1)/(x1 - x0));
}

#endif
