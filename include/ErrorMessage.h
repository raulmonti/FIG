/* Leonardo Rodríguez */
#include <sstream>
#include "position.hh"

/**
 * @brief ErrorMessage: Provides the ability to save error messages and warnings.
 * @note Visitor class inherits this class, then every extension of Visitor
 * has the ability to save error messages.
 */
class ErrorMessage {
private:
    /// The stream that saves the messages
    stringstream msg;
    /// Is there an error?
    bool _has_errors;
    /// Is there a warning?
    bool _has_warnings;
public:
    ErrorMessage() :
        _has_errors {false}, _has_warnings {false} {}

    bool has_errors() const {
        return (_has_errors);
    }

    bool has_warnings() const {
        return (_has_warnings);
    }

    void put_error(const string& error) {
        _has_errors = true;
        msg << "[Error] " << error << std::endl;
    }

    void put_warning(const string& msg) {
        _has_warnings = true;
        this->msg << "[Warning] " << msg << std::endl;
    }

    void ignore_errors() {
        _has_errors = false;
        _has_warnings = false;
        msg.str(std::string());
    }

    string get_msg() {
        return (msg.str());
    }
};
