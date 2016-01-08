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

#include <exception>
#include <utility>
#include <string>


namespace fig
{

/// Custom exception, use through macro defined at the end of this file
class FigException : public std::exception
{
	std::string msg_;

	inline virtual const char* what() const noexcept
	{
		return std::string("FigException raised: ").append(msg_).c_str();
	}

	inline void compose_msg(const char* file, int line)
	{
		msg_.append(" @ ").append(file).append(":").append(std::to_string(line));
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

#define  throw_FigException(msg) throw FigException((msg), __FILE__, __LINE__)

}

#endif // FIGEXCEPTION_H
