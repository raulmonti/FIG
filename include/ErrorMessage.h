#include <sstream>
#include "position.hh"

class ErrorMessage {
private:
    stringstream msg;
    bool _has_errors;
public:
    ErrorMessage() :
        _has_errors {false} {}

    bool has_errors() const {
        return (_has_errors);
    }

    void put_error(const string& error) {
        _has_errors = true;
        msg << "[Error] " << error << std::endl;
    }

    void put_error(const string& error, ModelParserGen::position position) {
        put_error(error);
        msg << "\t" << position << std::endl;
    }

    void put_msg(string msg) {
        this->msg << "[Info] " << msg << endl;
    }

    string get_msg() {
        return (msg.str());
    }
};
