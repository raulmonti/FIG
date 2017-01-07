#ifndef ARRAY_FUNCTION_H
#define ARRAY_FUNCTION_H

#include <random>
#include <cassert>

#include "exprtk.hpp"


template<typename T>
using parameter_list_t = typename exprtk::igeneric_function<T>::parameter_list_t;

template<typename T>
using generic_type = typename exprtk::igeneric_function<T>::generic_type;

template<typename T>
using vector_t  = typename generic_type<T>::vector_view;

template<typename T>
using scalar_t  = typename generic_type<T>::scalar_view;

/// fsteq(array, E) = first j such that array[j]==E, or -1 if no such j exists
template<typename T>
struct FstEqFunction : public exprtk::igeneric_function<T> {
    FstEqFunction() : exprtk::igeneric_function<T>("VT") {}

    inline T operator()(parameter_list_t<T> parameters) {
        generic_type<T> &gt = parameters[0];
        scalar_t<T> value (parameters[1]);
        vector_t<T> vector(gt);
        for (size_t j = 0; j < vector.size(); j++) {
            if (vector[j] == value()) {
                return T(j);
            }
        }
        return T(-1);
    }
};

/// lsteq(array, E) = last (greatest) j such that array[j]==E, or -1 if no such j exists
template<typename T>
struct LstEqFunction : public exprtk::igeneric_function<T> {
    LstEqFunction() : exprtk::igeneric_function<T>("VT") {}

    inline T operator()(parameter_list_t<T> parameters) {
        generic_type<T> &gt = parameters[0];
        scalar_t<T> value (parameters[1]);
        vector_t<T> vector(gt);
        if (vector.size() == 0) {
            // (unsigned int) 0 - 1 = caos
            // we do not support empty arrays anyway.
            return (-1);
        }
        for (size_t j = vector.size() - 1; j >= 0; j--) {
            if (vector[j] == value()) {
                return T(j);
            }
        }
        return T(-1);
    }
};

/// rndeq(array, E) = some j such that array[j]==E, or -1 if no such j exists
template<typename T>
struct RndEqFunction : public exprtk::igeneric_function<T> {
    ///@todo should I use the same generator than Clock ?
    /// Clock does not offer access to the generator.
    std::random_device rd;
    std::mt19937 gen;

    RndEqFunction() : exprtk::igeneric_function<T>("VT") {
        new (&rd) std::random_device();
        new (&gen) std::mt19937 (rd());
    }

    inline T operator()(parameter_list_t<T> parameters) {
        generic_type<T> &gt = parameters[0];
        scalar_t<T> value (parameters[1]);
        vector_t<T> vector(gt);
        std::vector<size_t> positions;
        //collect positions
        for (size_t j = 0; j < vector.size(); j--) {
            if (vector[j] == value()) {
                positions.push_back(j);
            }
        }
        if (positions.empty()) {
            return (-1);
        }
        //choose a position
        std::uniform_int_distribution<> dis(0, positions.size() - 1);
        size_t selected = dis(gen);
        //return the selected position
        return T(positions[selected]);
    }
};

/// minfrom(array, j) : <Min i : j <= i < array.size : i>
/// position of the minimum value of array[j...)
///  assert(0 <= j < array.size)  assert(array.size > 0)
template<typename T>
struct MinFromFunction : public exprtk::igeneric_function<T> {

    MinFromFunction() : exprtk::igeneric_function<T>("VT") {}

    inline T operator()(parameter_list_t<T> parameters) {
        generic_type<T> &gt = parameters[0];
        scalar_t<T> value (parameters[1]);
        vector_t<T> vector(gt);
        assert(vector.size() > 0); //non empty
        size_t pos;
        assert(value.to_uint(pos)); //keep it!
        assert(pos < vector.size());
        size_t selected = pos;
        T min = vector[selected];
        for (size_t i = pos + 1; i < vector.size(); i++) {
            if (vector[i] < min) {
                selected = i;
                min = vector[selected];
            }
        }
        return T(selected);
    }
};

/// maxfrom(array, j) : <Max i : j <= i < array.size : i>
/// position of the maximum value of array[j...)
///  assert(0 <= j < array.size)  assert(array.size > 0)
template<typename T>
struct MaxFromFunction : public exprtk::igeneric_function<T> {

    MaxFromFunction() : exprtk::igeneric_function<T>("VT") {}

    inline T operator()(parameter_list_t<T> parameters) {
        generic_type<T> &gt = parameters[0];
        scalar_t<T> value (parameters[1]);
        vector_t<T> vector(gt);
        assert(vector.size() > 0); //non empty
        size_t pos;
        assert(value.to_uint(pos)); //keep it!
        assert(pos < vector.size());
        size_t selected = pos;
        T max = vector[selected];
        for (size_t i = pos + 1; i < vector.size(); i++) {
            if (max < vector[i]) {
                selected = i;
                max = vector[selected];
            }
        }
        return T(selected);
    }
};

///  sumfrom(array, j) : <Sum i : j <= i < array.size : array[i]>
///  returns array[j] + array[j + 1] + ... + array[array.size() - 1]
template<typename T>
struct SumFromFunction : public exprtk::igeneric_function<T> {

    SumFromFunction() : exprtk::igeneric_function<T>("VT") {}
    inline T operator()(parameter_list_t<T> parameters) {
        generic_type<T> &gt = parameters[0];
        scalar_t<T> value (parameters[1]);
        vector_t<T> vector(gt);
        size_t pos;
        assert(value.to_uint(pos)); //keep it!
        assert(pos < vector.size());
        T sum  = 0;
        for (size_t i = pos; i < vector.size(); i++) {
            sum += vector[i];
        }
        return (sum);
    }
};

///  consec(array, k)==1 if there is a sequence
///  of consecutive numbers i1,....,ik such that
///  array[i1] && .... && array[ik] holds (0 otherwise)
template<typename T>
struct ConsecFunction : public exprtk::igeneric_function<T> {

    ConsecFunction() : exprtk::igeneric_function<T>("VT") {}

    inline T operator()(parameter_list_t<T> parameters) {
        generic_type<T> &gt = parameters[0];
        scalar_t<T> value (parameters[1]);
        vector_t<T> vector (gt);
        bool found = false;
        size_t i = 0;
        T k = value();
        while (i < vector.size() && !found) {
            T count = T(0);
            for (size_t j = i; j < vector.size() && j < i + k; j++) {
                if (!vector[j]) {
                    break;
                }
                count++;
            }
            found = (count == k);
            i++;
        }
        return T(found ? 1 : 0);
    }
};


/// broken(array, j) : array[j]=1; array[i]++ if array[i] != 0 and j!=i
/// @returns 0
/// @note modifies the array.
template<typename T>
struct BrokenFunction : public exprtk::igeneric_function<T> {

    BrokenFunction() : exprtk::igeneric_function<T>("VT") {}

    inline T operator()(parameter_list_t<T> parameters) {
        generic_type<T> &gt = parameters[0];
        scalar_t<T> value (parameters[1]);
        vector_t<T> vector(gt);
        unsigned int pos;
        assert(value.to_uint(pos)); //keep it!
        assert(pos < vector.size());
        vector[pos] = 1;
        for (size_t i = 0; i < vector.size(); i++) {
            if (vector[i] != 0 && i != pos) {
                vector[i]++;
            }
        }
        return (0);
    }
};


#endif
