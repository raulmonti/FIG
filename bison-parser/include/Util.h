#ifndef UTIL_H
#define UTIL_H
#include <memory>
#include <vector>

using namespace std;

template<typename T>
using vector_ptr = vector<unique_ptr<T>>;

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T>
vector<T> concat(vector<T> &v1, const vector<T> &v2) {
    v1.insert(v1.end(), v2.begin(), v2.end());
    return v1;
}


#endif
