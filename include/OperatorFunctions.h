#ifndef OP_FUN_H
#define OP_FUN_H
#include <cmath>

namespace binaryfun {

inline int plus (int x, int y) {
    return (x + y);
}

inline int times (int x, int y) {
    return (x * y);
}

inline int minus (int x, int y) {
    return (x - y);
}

inline int div (int x, int y) {
    return (x / y);
}

inline int mod (int x, int y) {
    return (x % y);
}

inline int andd (int x, int y) {
    return (x && y);
}

inline int orr (int x, int y) {
    return (x || y);
}

inline int implies (int x, int y) {
    return (!x || y);
}

inline int eq (int x, int y) {
    return (x == y);
}

inline int neq (int x, int y) {
    return (x != y);
}

inline int lt (int x, int y) {
    return (x < y);
}

inline int gt (int x, int y) {
    return (x > y);
}

inline int le (int x, int y) {
    return (x <= y);
}

inline int ge (int x, int y) {
    return (x >= y);
}

inline int min (int x, int y) {
    return (std::min(x, y));
}

inline int max (int x, int y) {
    return (std::max(x, y));
}

inline int pow(int x, int y) {
    return (std::pow(x, y));
}

} //namespace binaryfun

namespace unaryfun {

inline int nott (int x) {
    return (!x);
}

inline int minus (int x) {
    return (-x);
}

inline int abs(int x) {
    return (std::abs(x));
}

inline int sgn(int x) {
    return (x > 0) - (x < 0);
}

} //namespace unaryfun


#endif
