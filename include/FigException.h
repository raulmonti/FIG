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

namespace fig
{

/// Very primitive exception support
class FigException : public std::exception
{
	std::string msg_;
	inline virtual const char* what() const throw()
	{
		return std::string("FigException raised: ").append(msg_).c_str();
	}
public:
	FigException(const char* msg) : msg_(msg) {}
	FigException(const std::string& msg) : msg_(msg) {}
	FigException(std::string&& msg) : msg_(std::move(msg)) {}
	inline const std::string& msg() { return msg_; }
};

}

#endif // FIGEXCEPTION_H
