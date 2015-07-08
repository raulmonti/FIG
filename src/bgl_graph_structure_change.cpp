#include <iostream>    // std::cout
#include <string>      // std::string
#include <vector>      // std:::vector<>
#include <tuple>       // std::tuple<>
#include <utility>     // std::pair
#include <algorithm>   // std::for_each, std::swap
#include <unistd.h>    // EXIT_{SUCCESS,FAILURE}
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <boost/graph/compressed_sparse_row_graph.hpp>  // TODO: make available
// http://stackoverflow.com/questions/8486077/how-to-compile-link-boost-with-clang-libc


int main (int, char**)
{
	// TODO
	//     Create adjacency list graph and transform it into a CSR graph
	//     Compile also with Clang for better debugging messages
	//     Print both structures and compare (check boost::print_graph)
	//     Study BGL visitors?  http://www.boost.org/doc/libs/1_58_0/libs/graph/doc/DijkstraVisitor.html

	return EXIT_SUCCESS;
}
