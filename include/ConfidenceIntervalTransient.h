//==============================================================================
//
//  ConfidenceIntervalTransient.h
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

#ifndef CONFIDENCEINTERVALTRANSIENT_H
#define CONFIDENCEINTERVALTRANSIENT_H

// C++
#include <vector>
// FIG
#include <ConfidenceInterval.h>


namespace fig
{

class ConfidenceIntervalTransient : public ConfidenceInterval
{

	double M2, numRares_, logNumSamples_;

public:  // Ctor

	/// @copydoc ConfidenceInterval::ConfidenceInterval()
	ConfidenceIntervalTransient(double confidence,
								double precision,
								bool dynamicPrecision = false,
								bool neverStop = false);
public:  // Modifyers

	/// @param wighedNRE Weighed number of rare events from last simulation
	void update(const double& weighedNRE) override;

	/// Stub to update(), multiple value feeding isn't tolerated yet
	void update(const double& numRE, const double& numNewExp) override;

	/// @todo TODO write docstring
	void update(const std::vector<double> &numREs);

public:  // Utils

	bool min_samples_covered() const noexcept override;

	double precision(const double& confco) const override;

	void reset(bool fullReset = false) noexcept override;
};

}  // namespace fig

#endif // CONFIDENCEINTERVALTRANSIENT_H
