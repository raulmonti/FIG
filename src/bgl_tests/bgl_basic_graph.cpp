#include <iostream>                  // for std::cout
#include <utility>                   // for std::pair
#include <algorithm>                 // for std::for_each
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>


/**
 * @brief Functor checking the adjacency from one vertex's perspective
 */
template <class Graph> struct vertices_adjacency
{
	typedef typename boost::graph_traits<Graph>::vertex_descriptor  Vertex;
	typedef typename boost::graph_traits<Graph>::edge_descriptor    Edge;

	vertices_adjacency(Graph& g_) : g(g_) {}

	void operator()(const Vertex& v) const
	{
		typename boost::property_map<Graph, boost::vertex_index_t>::type
			index = boost::get(boost::vertex_index, g);

		std::cout << "Vertex " << index[v] << " adjacencies:" << std::endl;

		std::cout << " adjacent vertices:";  // considers out-edges only
		typename boost::graph_traits<Graph>::adjacency_iterator ait, ait_end;
		for (boost::tie(ait, ait_end) = boost::adjacent_vertices(v, g)
			; ait != ait_end
			; ait++)
		{
			std::cout << " " << index[*ait];
		}
		std::cout << std::endl;

		std::cout << "  out-edges:";
		typename boost::graph_traits<Graph>::out_edge_iterator oeit, oeit_end;
		for (boost::tie(oeit, oeit_end) = boost::out_edges(v, g)
			; oeit != oeit_end
			; oeit++)
		{
			Edge e = *oeit;
			Vertex src = boost::source(e,g),
				   trg = boost::target(e,g);
			std::cout << " (" << index[src] << "," << index[trg] << ")";
		}
		std::cout << std::endl;

		std::cout << "   in-edges:";
		typename boost::graph_traits<Graph>::in_edge_iterator ieit, ieit_end;
		for (boost::tie(ieit, ieit_end) = boost::in_edges(v, g)
			; ieit != ieit_end
			; ieit++)
		{
			Edge e = *ieit;
			Vertex src = boost::source(e,g),
				   trg = boost::target(e,g);
			std::cout << " (" << index[src] << "," << index[trg] << ")";
		}
		std::cout << std::endl;
	}

	Graph& g;
};


/**
 * @brief Fuck off
 */
int main(int, char**)
{
	// create a typedef for the Graph type
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS> Graph;
	typedef boost::graph_traits<Graph>::vertex_iterator vertex_iterator;
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

	// Define the graph with all edges at once
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
	boost::graph_traits<Graph>::edge_iterator eit, eit_end;
	for (boost::tie(eit, eit_end) = boost::edges(g) ; eit != eit_end ; eit++) {
		// notice alternative pair iteration with boost::tie
		boost::graph_traits<Graph>::edge_descriptor edge = *eit;
		std::cout << " (" << index[boost::source(edge, g)]
				  <<  "," << index[boost::target(edge, g)] << ")";
	}
	std::cout << std::endl;

	// See adjacency from each vertex's perspective
	std::for_each(boost::vertices(g).first,
				  boost::vertices(g).second,
				  vertices_adjacency<Graph>(g));

	return 0;
}
