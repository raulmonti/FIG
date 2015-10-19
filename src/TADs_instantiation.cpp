/**
 * Instantiation of all basic TADs for debugging purposes.
 * This file is part of the FIG project.
 */

// C++
#include <iostream>
// C
#include <cassert>
// FIG
#include <Clock.h>  // Clock

int main()
{
	// Clocks
	fig::ParamList< fig::CLOCK_INTERNAL_TYPE > list = {{10.0, 5.0}};
	std::cout << fig::distributions_list.at("uniform01")(list) << std::endl;
	//fig::Clock c(list, fig::distributions_list["uniform"]);
	//assert(0.0 != c.sample() || 0.0 != c());  // exercise object
	return 0;
}
