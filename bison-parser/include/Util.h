#ifndef UTIL_H
#define UTIL_H

#include <memory>
#include <vector>
#include <sstream>

using std::forward;
using std::unique_ptr;
using std::stringstream;
using std::string;
using std::endl;
using std::vector;
using std::shared_ptr;

template<typename T>
using shared_vector = std::vector<shared_ptr<T>>;

template<typename K, typename T>
using shared_map = std::map<K, shared_ptr<T>>;

class ErrorMessage {
private:
    stringstream msg;
    bool _has_errors;
public:
    ErrorMessage() :
	_has_errors {false} {};
    
    bool has_errors() const {
	return (_has_errors);
    }
    
    void put_error(string error) {
	_has_errors = true;
	msg << "[Error] " << error << endl;
    }
    
    void put_msg(string msg) {
	this->msg << "[Info] " << msg << endl;
    }
    
    string get_msg() {
	return (msg.str());
    }
};

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

//standard in c++14
template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif
