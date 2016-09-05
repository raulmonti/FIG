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
#include <memory>

class ModelAST;

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
public: // Offered data

	/// Last parsed IOSA model, or nullptr if none
	static std::shared_ptr<ModelAST> modelAST;

public: // Offered facilities

	/**
	 * Translate existing IOSA model file to <a href="http://jani-spec.org/">
	 * JANI specification format</a>. If specified, include also all properties
	 * defined in the properties file.
	 *
	 * @param iosaModelFile Path to (or name of) file with the IOSA model
	 * @param iosaPropsFile Path to (or name of) file with properties to check
	 * @param janiFilename  Desired name of the translated JANI file to create
	 * @param validityCheck Whether to validate the IOSA syntax of the model file
	 *
	 * @return Name of file with the model translated to an STA written in
	 *         the JANI specification format (see notes)
	 *
	 * @throw FigException if we couldn't generate a valid JANI translation
	 *
	 * @note IOSA model files can also have properties defined in them,
	 *       inside a "properties...endproperties" section.
	 * @note If 'janiFilename' was passed then the JANI file generated will
	 *       have that name. Otherwise a name related to 'iosaModelFile' is
	 *       automatically generated.
	 */
	static std::string
	IOSA_2_JANI(const std::string& iosaModelFile,
				const std::string& iosaPropsFile = "",
				const std::string& janiFilename = "",
				bool validityCheck = true);

	/**
	 * Translate model file specified in <a href="http://jani-spec.org/">
	 * JANI format</a> to IOSA syntax. Properties, if present, will be included
	 * inside a "properties...endproperties" section of the IOSA file.
	 *
	 * @param janiModelFile Path to (or name of) file with STA model
	 *                      written in valid JANI-spec format
	 * @param iosaFilename  Desired name of the translated IOSA file to create
	 *
	 * @return Name of file with the model translated to IOSA syntax (see notes)
	 *
	 * @throw FigException if we couldn't generate a valid IOSA translation,
	 *                     e.g. when the JANI model wasn't a deterministic STA
	 *
	 * @note If 'iosaFilename' was passed then the IOSA file generated will
	 *       have that name. Otherwise a name related to 'janiModelFile' is
	 *       automatically generated.
	 */
	static std::string
	JANI_2_IOSA(const std::string& janiModelFile,
				const std::string& iosaFilename = "");
};

} // namespace fig

#endif // JANI_TRANSLATOR_H
