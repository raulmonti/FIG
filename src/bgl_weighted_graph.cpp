#include <iostream>    // std::cout
#include <string>      // std::string
#include <utility>     // std::pair
#include <algorithm>   // std::for_each
#include <unistd.h>    // EXIT_{SUCCESS,FAILURE}
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

int main(int, char**)
{
	typedef boost::property<boost::vertex_distance_t, float,
			boost::property<boost::vertex_name_t, std::string>>  VertexProperty;
	typedef boost::property<boost::edge_weight_t, float>         EdgeProperty;
	typedef boost::adjacency_list<
		boost::listS, boost::vecS, boost::directedS,
		VertexProperty, EdgeProperty> Graph;
	typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
	typedef boost::graph_traits<Graph>::edge_descriptor   Edge;
	typedef std::pair<int,int> E;

	const int num_nodes = 5;
	E edges[] = { E(0,2),
				  E(1,1), E(1,3), E(1,4),
				  E(2,1), E(2,3),
				  E(3,4),
				  E(4,0), E(4,1) };
	int weights[] = { 1, 2, 1, 2, 7, 3, 1, 1, 1};

	Graph G(edges, edges + sizeof(edges) / sizeof(E), weights, num_nodes);

	return EXIT_SUCCESS;
}
