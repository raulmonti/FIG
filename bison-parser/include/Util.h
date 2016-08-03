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

class Log {
    stringstream msg;
    bool _has_errors;
public:
    Log() {};
    Log(const Log &) = delete;
    Log(Log &&) = delete;
    
    bool has_errors() const {
	return (_has_errors);
    }

    void put_error(string error) {
	_has_errors = true;
	msg << "[Error] " << error << endl;
    }
    
    void pur_msg(string msg) {
	this->msg << "[Info] " << msg << endl;
    }

    string get_msg() {
	return (msg.str());
    }
};

/*
template<typename T>
using vector_ptr = vector<unique_ptr<T>>;

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
*/

template<typename T>
vector<T> concat(vector<T> &v1, const vector<T> &v2) {
    v1.insert(v1.end(), v2.begin(), v2.end());
    return v1;
}


#endif
