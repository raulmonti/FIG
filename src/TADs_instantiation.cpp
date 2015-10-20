/**
 * Instantiation of all basic TADs for debugging purposes.
 * This file is part of the FIG project.
 */

// C++
#include <iostream>
#include <exception>
// C
#include <cassert>
// FIG
#include <Clock.h>  // Clock

int main()
{
	// Clocks
	fig::DistributionParameters params = {{2.0, 5.0}};
	fig::Clock c("uniformAB", params);
	assert(0.0 != c.sample() || 0.0 != c());  // exercise object
	try { fig::Clock c("unexistent_distribution", params); }
	catch (std::out_of_range e) { /* this was expected */ }

	return 0;
}
