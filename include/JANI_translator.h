//==============================================================================
//
//  JANI_translator.h
//
//  Copyleft 2016-
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

#ifndef JANI_TRANSLATOR_H
#define JANI_TRANSLATOR_H

#include <string>
#include <fstream>


namespace fig
{

/// Transform Stochastic Automata model specification files between
/// <a href="http://jani-spec.org/">JANI format</a> and IOSA syntax.<br>
/// From JANI's point of view the files correspond to
/// <a href="http://goo.gl/vros8C">Stochatic Timed Automata</a> (STA),
/// of which only a deterministic subset (viz. without non-determinism)
/// is valid for translation.<br>
/// From FIG's point of view the files correspond to
/// <a href="http://dsg.famaf.unc.edu.ar/node/643">Input/Output Stochastic
/// Automata</a> (IOSA).
class JaniTranslator
{
public:

	/// Translate IOSA model file to <a href="http://jani-spec.org/">
	/// JANI specification format</a>
	/// @param iosaModelFile Open stream of the file with the IOSA model
	/// @param validityCheck Whether to validate the IOSA syntax of the model file
	/// @param janiFilename  Desired name of the translated JANI file to create
	/// @return Valid stream to a file with the model translated to an STA
	///         written in the JANI specification format
	/// @throw FigException if we couldn't generate a valid JANI translation
	std::fstream
	IOSA_2_JANI(const std::ifstream& iosaModelFile,
				bool validityCheck = true,
				const std::string& janiFilename = "IOSA_model.jani");

	/// @copybrief IOSA_2_JANI(const std::ifstream&,bool)
	/// @details When generating the translated JANI file,
	///          include the properties defined in "iosaPropsFile"
	/// @param iosaPropsFile Open stream of the file with the properties to check
	/// @copydetails IOSA_2_JANI(const std::ifstream&,bool)
	/// @note The IOSA model files can also have properties defined in them,
	///       inside a "properties...endproperties" section.
	std::fstream
	IOSA_2_JANI(const std::ifstream& iosaModelFile,
				const std::ifstream& iosaPropsFile,
				bool validityCheck = true,
				const std::string& janiFilename = "IOSA_model.jani");

	/// @copybrief IOSA_2_JANI(const std::ifstream&,bool)
	/// @param iosaModelFile Path to a file with the IOSA model
	/// @param iosaPropsFile Path to a file with properties to check
	/// @param validityCheck Whether to validate the IOSA syntax of the model file
	/// @return Name of file with the model translated to an STA written in
	///         the JANI specification format, created in the same directory
	///         where the "iosaModelFile" is located
	/// @throw FigException if we couldn't generate a valid JANI translation
	std::string
	IOSA_2_JANI(const std::string& iosaModelFile,
				const std::string& iosaPropsFile = "",
				bool validityCheck = true);

	/// Translate model file specified in <a href="http://jani-spec.org/">
	/// JANI format</a> to IOSA syntax.
	/// @param janiModelFile Open stream of file with STA model written in
	///                      valid JANI-spec format
	/// @param iosaFilename  Desired name of the translated IOSA file to create
	/// @return Valid stream to a file with the model translated to IOSA syntax
	/// @throw FigException if we couldn't generate a valid IOSA translation,
	///                     e.g. when the JANI model wasn't a deterministic STA
	std::fstream JANI_2_IOSA(const std::ifstream& janiModelFile,
							 const std::string& iosaFilename = "JANI_model.sa");

	/// @copybrief JANI_2_IOSA(const std::ifstream&)
	/// @param janiModelFile Path to file with STA model written in
	///                      valid JANI-spec format
	/// @return Name of file with the model translated to IOSA syntax, created
	///         in the same directory where the "iosaModelFile" is located
	/// @throw FigException if we couldn't generate a valid IOSA translation,
	///                     e.g. when the JANI model wasn't a deterministic STA
	std::string
	JANI_2_IOSA(const std::string& janiModelFile);
};

} // namespace fig

#endif // JANI_TRANSLATOR_H
