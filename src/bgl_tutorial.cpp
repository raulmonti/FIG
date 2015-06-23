#include <iostream>                  // for std::cout
#include <utility>                   // for std::pair
#include <algorithm>                 // for std::for_each
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>

/**
 * @brief Fuck off
 */
int main(int, char**)
{
	// create a typedef for the Graph type
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS> Graph;
	typedef boost::graph_traits<Graph>::vertex_iterator vertex_iterator;
	typedef boost::graph_traits<Graph>::edge_iterator   edge_iterator;
	typedef boost::property_map<Graph, boost::vertex_index_t>::type IndexMap;

	// Make convenient labels for the vertices
	enum { A, B, C, D, E, N };
	const int num_vertices = N;
	const char name[] = "ABCDE";

	// writing out the edges in the graph
	typedef std::pair<int, int> Edge;
	Edge edge_array[] = { Edge(A,B),
						  Edge(A,D),
						  Edge(C,A),
						  Edge(D,C),
						  Edge(C,E),
						  Edge(B,D),
						  Edge(D,E) };

//	// declare a graph object
//	Graph g(num_vertices);
//	// add the edges to the graph object
//	for (int i = 0; i < num_edges; ++i) {
//		add_edge(edge_array[i].first, edge_array[i].second, g);
//	}
	// define the graph with all edges at once
	Graph g(edge_array, edge_array + sizeof(edge_array) / sizeof(Edge), num_vertices);

	// Get the property map for vertex indices
	IndexMap index = boost::get(boost::vertex_index, g);

	// Show vertices
	std::cout << "Num vertices: " << boost::num_vertices(g) << std::endl;
	std::cout << "Vertices:";
	std::pair<vertex_iterator,vertex_iterator> vit = boost::vertices(g);
	for (; vit.first < vit.second ; vit.first++) {
		boost::graph_traits<Graph>::vertex_descriptor vertex = *vit.first;
		std::cout << " " << index[vertex];
	}
	std::cout << std::endl;

	// Show edges
	std::cout << "Num edges: " << boost::num_edges(g) << std::endl;
	std::cout << "Edges:";
//	std::pair<edge_iterator,edge_iterator> eit = boost::edges(g);
//	for (; eit.first != eit.second ; eit.first++) {
//		boost::graph_traits<Graph>::edge_descriptor edge = *eit.first;
//		std::cout << " (" << index[boost::source(edge, g)]
//				  <<  "," << index[boost::target(edge, g)] << ")";
//	}
	edge_iterator eit, eit_end;
	for (boost::tie(eit, eit_end) = boost::edges(g) ; eit != eit_end ; eit++) {
		boost::graph_traits<Graph>::edge_descriptor edge = *eit;
		std::cout << " (" << index[boost::source(edge, g)]
				  <<  "," << index[boost::target(edge, g)] << ")";
	}
	std::cout << std::endl;


	return 0;
}
