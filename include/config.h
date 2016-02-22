//==============================================================================
//
// Configuration file for FIG project 'Parser module'.
// Raul Monti
// 2015
//
//==============================================================================


#ifndef CONFIG_H
#define CONFIG_H

#include <sstream>


//#define __PARSER_VERBOSE__
//#define __PARSER_DEBUG__

#ifdef __PARSER_DEBUG__
#define __debug__(x) std::cerr << x;
#else
#define __debug__(x) ;
#endif


#ifdef __PARSER_VERBOSE__
#define pout std::cout
#else
#define pout dummy_out
#endif

// Warning output
#define wout std::cout
// Testing output
#define tout std::cout

using namespace std;

namespace parser{

static stringstream dummy_out;

}



#endif // CONFIG_H
