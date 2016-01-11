//==============================================================================
//
//  FigException.h
//
//  Copyleft 2015-
//  Authors:
//  - Carlos E. Budde <cbudde@famaf.unc.edu.ar> (Universidad Nacional de Córdoba)
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


#ifndef FIGEXCEPTION_H
#define FIGEXCEPTION_H

// C
#include <libgen.h>  // basename(), dirname()
#include <cstring>   // strndup()
// C++
#include <exception>
#include <utility>
#include <string>


namespace fig
{

/// Custom exception, use through macro defined at the end of this file
class FigException : public std::exception
{
	std::string msg_;

	inline virtual const char* what() const noexcept { return msg_.c_str(); }

	inline void compose_msg(const char* fullpath0, int line)
	{
		const size_t maxlen(1u<<7);
		auto fullpath1 = strndup(fullpath0, maxlen);
		auto fullpath2 = strndup(fullpath0, maxlen);
		msg_ = msg_.append(" @ ").append(basename(fullpath1))
				   .append(":").append(std::to_string(line))
				   .append("\nThrown from dir ").append(dirname(fullpath2));
		free(fullpath1);
		free(fullpath2);
	}

public:

	FigException(const char* msg, const char* file, int line) :
		msg_(msg)
		{ compose_msg(file, line); }

	FigException(const std::string& msg, const char* file, int line) :
		msg_(msg)
		{ compose_msg(file, line);}

	FigException(std::string&& msg, const char* file, int line) :
		msg_(std::move(msg))
		{ compose_msg(file, line); }

	inline const std::string& msg() { return msg_; }
};

} // namespace fig


#define  throw_FigException(msg) throw fig::FigException((msg), __FILE__, __LINE__)


#endif // FIGEXCEPTION_H
