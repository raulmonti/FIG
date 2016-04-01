//==============================================================================
//
//  string_utils.h
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

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <string>
#include <vector>


/**
 * @brief Replace in "s" all occurrences of "from" for "to"
 * @param s    String to have the substitutions made
 * @param from Substring from "s" to search for
 * @param to   String for which "from" will be replaced in "str"
 * @return Resulting string (i.e. 's' after the call)
 * @note Taken from <a href="http://stackoverflow.com/a/3418285">
 *       this SO answer by Michael Mrozek</a>.
 */
std::string& replace_substring(std::string& s,
							   const std::string& from,
							   const std::string& to);
/// @copydoc replace_substring()
std::string replace_substring(std::string&& s,
							  const std::string& from,
							  const std::string& to);

/**
 * @brief Delete in "s" all occurrences of "substr"
 * @param s      String to have the deletions made
 * @param substr Substring from "s" to search for
 * @return Resulting string (i.e. 's' after the call)
 */
std::string& delete_substring(std::string& s, const std::string& substr);
/// @copydoc delete_substring()
std::string delete_substring(std::string&& s, const std::string& substr);

/**
 * @brief Remove whitespace from begin and end of string
 * @note Taken from <a href="http://stackoverflow.com/a/17976541">
 *       this SO answer by David G.</a>
 */
std::string trim(const std::string &s);

/// @copydoc trim
std::string trim(std::string&& s);

/**
 * @brief Split a string on every occurrence of the specified delimiter
 * @param s     String to split
 * @param delim Delimiter to chop the string by
 * @return Vector with chopped contents of the original string.
 * @note Every occurrence of the delimiter in the original string
 *       is deleted and will not occur in the result.
 * @note Taken from <a href="http://stackoverflow.com/a/236803">
 *       this SO community-wiki answer</a>.
 */
std::vector<std::string> split(const std::string &s, char delim);

#endif // STRING_UTILS_H
