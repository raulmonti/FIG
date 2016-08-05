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

//use to track msg errors
//@todo use some of the Fig already defined logs?
class Log {
private:
    stringstream msg;
    bool _has_errors;
    Log() : _has_errors {false} {};
public:
    static Log &get_instance() {
	static Log instance;
	return (instance);
    }
    Log(const Log &) = delete;
    Log(Log &&) = delete;
    
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


#endif
