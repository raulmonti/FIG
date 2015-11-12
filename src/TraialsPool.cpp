//==============================================================================
//
//  TraialsPool.cpp
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


#include <TraialsPool.h>


namespace fig
{

std::unique_ptr< TraialPool > TraialPool::instance_ = nullptr;

size_t TraialPool::initialSize_ = (1u) << 12;  // 4K

size_t TraialPool::sizeIncrement_ = TraialPool::initialSize_ / 10;

/// @todo implement methods below the comment "Access to resources" in header
///       according to documentation in place

} // namespace fig
