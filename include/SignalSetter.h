//==============================================================================
//
//  SignalSetter.h
//
//  Copyleft 2016-
//  Authors:
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
//  - Matthieu M. (most of the code in this file was taken from the following
//                 SO answer by Matthieu: http://stackoverflow.com/a/10816266)
//
//------------------------------------------------------------------------------
//
//  This file is part of FIG.
//
//  The Finite Improbability Generator (FIG) project is free software;
//  you can redistribute it and/or modify it under the terms of the GNU
//  General Public License as published by the Free Software Foundation;
//  either version 3 of the License, or (at your option) any later version.
//
//  FIG is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with FIG; if not, write to the Free Software Foundation,
//	Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//==============================================================================


#ifndef SIGNALSETTER_H
#define SIGNALSETTER_H

// C
#include <csignal>  // std::signal()
// C++
#include <array>
#include <utility>  // std::swap()
#include <functional>


namespace fig
{

/// Our personal C++ signal handling signature,
/// way more versatile than the POSIX-C standard
typedef std::function<void(int)> SignalHandlerType;

/// Highest index among the handled signals (SIGINT=2, SIGALRM=14, SIGTERM=15)
constexpr int MAX_SIGNUM_HANDLED = 15;
// needs c++14: constexpr int MAX_SIGNUM_HANDLED = std::max<int>({SIGINT, SIGALRM, SIGTERM});

/// Must be defined in the calling thread's compile unit (i.e. the cpp file)
/// @note Idea for multiple handlers taken from
///       <a href="http://stackoverflow.com/a/10819579">this SO answer by
///       Arpegius</a>.
extern thread_local std::array< SignalHandlerType, MAX_SIGNUM_HANDLED > SignalHandlers;

/// C API's signature for signal handling, used by SignalSetter
void handle_signal(int const i)
{
	if (SignalHandlers[i-1])
		SignalHandlers[i-1](i);
}

/**
 * @brief The SignalSetter class was designed to allow the binding
 *        of C++ lambda functions as C signal handlers.
 *
 *        The idea is to wrap the desired behaviour in a properly typed
 *        lambda function: all captures must be explicitly specified
 *        and the function signature must be "[...] (const int) -> void".
 *        Notice this also allows the use of class member functions.
 *
 *        However, as stated in <a href="http://stackoverflow.com/a/10816266">
 *        this SO answer by Matthieu</a>, the thus provided handler isn't safe
 *        in the way POSIX-C requests. So be careful when implementing delicate
 *        code, and try to provide as small a handler as possible.
 *
 * @code
 * // Following is a use example:
 *
 * bool interrupted(false);
 * std::string localVar("");
 *
 * // Your code here ...
 *
 * SignalSetter handler(SIGINT, [&interrupted, &localVar](const int) -> void
 *     {
 *          interrupted = true;
 *          localVar = "SIGINT";
 *     }
 * );
 *
 * // ... some more of your code here ...
 *
 * if (interrupted) {
 *     std::cerr << localVar << std::endl;
 *     exit(EXIT_FAILURE);
 * }
 * @endcode
 */
class SignalSetter
{
	/// C API's signature for signal handling
	typedef void(*CHandlerType)(int);

	/// POSIX-C signal (SIGINT, SIGABRT, SIGALRM, etc)
	int signal_;

	/// C API signal handler
	CHandlerType cHandler_;

	/// "C++ signal handler"
	SignalHandlerType handler_;

public:

	/// Forbid copies
	SignalSetter(const SignalSetter&) = delete;

	/// Forbid copies
	SignalSetter& operator=(const SignalSetter&) = delete;

	/// Data ctor
	/// @param signal @copydoc signal_
	/// @param sh     @copydoc handler_
	SignalSetter(int signal, SignalHandlerType&& sh):
		signal_(signal),
		cHandler_(0),
		handler_(sh)
	{
		assert(signal_ <= MAX_SIGNUM_HANDLED);
		cHandler_ = std::signal(signal_, &handle_signal);
		// Register this handler_ in its proper position
		std::swap(SignalHandlers[signal-1], handler_);
	}

	~SignalSetter() {
		std::signal(signal_, cHandler_);
		std::swap(SignalHandlers[signal_-1], handler_);
	}
};

} // namespace fig

#endif // SIGNALSETTER_H

