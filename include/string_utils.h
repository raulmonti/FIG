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


/// Count the number of times character 'c' appears in string 's'
size_t count(const std::string& s, const char& c);

/// Is \a substr a substring of \a str?
/// @note Case insensitive version taken from
///       <a href="http://stackoverflow.com/a/19839371">this SO answer by user CC.</a>
bool is_substring(const std::string& str,
                  const std::string& substr,
                  const bool caseSensitive = true);

/// Is \a prefix a prefix of \a str?
bool is_prefix(const std::string& str,
               const std::string& prefix,
               const bool caseSensitive = true);

/// Is \a suffix a suffix of \a str?
bool is_suffix(const std::string& str,
               const std::string& suffix,
               const bool caseSensitive = true);

/**
 * @brief Replace in \par s all occurrences of \par dis by \par todis
 * @param s    String to have the substitutions made
 * @param from Substring from "s" to search for
 * @param to   String for which "from" will be replaced in "str"
 * @return \par s after the call
 * @note Taken from <a href="http://stackoverflow.com/a/3418285">
 *       this SO answer by Michael Mrozek</a>.
 */
std::string& replace_substring(std::string& s,
                               const std::string& dis,
                               const std::string& todis);
/// @copydoc replace_substring()
std::string& replace_substring(std::string&& s,
                              const std::string& dis,
                              const std::string& todis);

/**
 * @brief Delete in \par s all occurrences of "substr"
 * @param s      String to have the deletions made
 * @param substr Substring from "s" to search for
 * @return \par s after the call
 */
std::string& delete_substring(std::string& s, const std::string& substr);
/// @copydoc delete_substring()
std::string& delete_substring(std::string&& s, const std::string& substr);
/**
 * @brief Compose a string just like "s" but with all occurrences of "substr"
 *        therein deleted
 * @param s      String to have the deletions made
 * @param substr Substring deleted from "s" to compose the result
 * @return Resulting string
 */
std::string delete_substring(const std::string& s, const std::string& substr);

/**
 * @brief Remove whitespace from begin and end of string
 * @note Taken from <a href="http://stackoverflow.com/a/17976541">
 *       this SO answer by David G.</a>
 */
std::string trim(const std::string &s);

/// @copydoc trim
std::string trim(std::string& s);

/// @copydoc trim
std::string trim(std::string&& s);

/**
 * @brief Split a string on every occurrence of the specified delimiter
 * @details The delitimer isn't included in the resulting substrings vector.
 *          Unless otherwise specified empty substrings are also skipped.
 *          For instance if the string "12,a,-5,,a" is split for the delimiter
 *          character ',' then the result will be ["12","a","-5","a"]
 * @param s     String to split
 * @param delim Delimiter to chop the string by (default is space)
 * @param includeEmptyMatches Whether to include empty strings in the result
 *                            (whenever found that is)
 * @return Vector with chopped contents of the original string.
 * @note Adapted from <a href="http://stackoverflow.com/a/236803">
 *       this SO community-wiki answer</a>.
 */
std::vector<std::string> split(const std::string &s,
							   char delim = ' ',
							   bool includeEmptyMatches = false);

/// Tells whether 'filename', interpreted as the (path to or) name of a file,
/// has the extension 'extension'
bool filename_has_extension(const std::string& filename,
							const std::string& extension);

/// Create a duplicate of 'filename' with its extension changed to 'extension'
/// @return 'filename' with new extension 'extension'
/// @note If 'filename' didn't contain any ".ext" final section,
///       then 'extension' is simply appended to 'filename'
std::string change_filename_extension(const std::string& filename,
									  const std::string& extension);

#endif // STRING_UTILS_H
