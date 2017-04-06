//==============================================================================
//
//  FigVersionVisitor
//
//  Copyleft 2017-
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
//------------------------------------------------------------------------------
//
//  This file is an extension to the Templatized C++ Command Line Parser
//  by Michael E. Smoot (TCLAP library, Copyright (c) 2003-2011)
//  All credit regarding this single file should go to him.
//
//==============================================================================

#ifndef FIGVERSIONVISITOR_H
#define FIGVERSIONVISITOR_H

// C++
#include <string>
#include <ostream>
// External code
#include <Visitor.h>
#include <ArgException.h>


namespace TCLAP
{

/// @brief   Mimic the original VersionVisitor from TCLAP for the FIG tool
/// @details Designed to print long version information via the "-v" or
///          "--vesion-full" switches, into a parameterized output stream.
class FigVersionVisitor : public Visitor
{
	private:
		/// Prevent accidental copying
		FigVersionVisitor(const FigVersionVisitor& rhs);
		/// @copydoc FigVersionVisitor(const FigVersionVisitor&)
		FigVersionVisitor& operator=(const FigVersionVisitor& rhs);

	protected:

		/// Output stream to print version info into
		std::ostream& _out;

		/// Long version info string
		const std::string _longVersionInfo;

	public:

		/// Ctor
		FigVersionVisitor(std::ostream& oStream,
						  const std::string& versionInfo) :
			_out(oStream),
			_longVersionInfo(versionInfo) {}

		/// Print long version info into the out-stream, and exit gracefully
		void visit()
		{
			_out << std::endl << _longVersionInfo << std::endl;
			throw ExitException(0);
		}
};

} // namespace TCLAP

#endif // FIGVERSIONVISITOR_H
