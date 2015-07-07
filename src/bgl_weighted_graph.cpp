#include <iostream>    // std::cout
#include <string>      // std::string
#include <vector>      // std:::vector<>
#include <tuple>       // std::tuple<>
#include <utility>     // std::pair
#include <algorithm>   // std::for_each, std::swap
#include <unistd.h>    // EXIT_{SUCCESS,FAILURE}
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>



namespace bgl = boost;

using std::string;

typedef std::pair<string,float> Pair;

class GraphException : public std::exception
{
	string msg;
	virtual const char* what() const throw() {
		return string("Exception raised: ").append(msg).c_str();
	}
public:
	GraphException(const char* _msg) : msg(_msg) {}
};


// ///////////////////////////////////////////////////////////////////////////
//
//	NOTE: will use Boost bundled properties (http://goo.gl/mJS8gP)
//
//	typedef bgl::property<bgl::vertex_distance_t, float,
//			bgl::property<bgl::vertex_name_t, std::string>>
//		VertexProperty;
//	typedef bgl::property<bgl::edge_weight_t, float>
//		EdgeProperty;
//	typedef bgl::adjacency_list<
//		bgl::listS, bgl::vecS, bgl::directedS,
//		VertexProperty, EdgeProperty> Graph;
//
struct Importance
{
	Importance() : name(""), importance(0) {}
	Importance(const string& name_, const float& importance_) :
		name(name_),
		importance(importance_)
	{}
	Importance(const Pair& imp) :
		name(imp.first),
		importance(imp.second)
	{}
	string name;
	float  importance;
};
struct Probability
{
	Probability() : weight(0.0) {}
	Probability(float w) : weight(w) {
		if (0.0 > w || w > 1.0)
			throw GraphException("probability values ∈ [0,1]");
	}
	Probability& operator=(Probability that) {
		std::swap(weight, that.weight);
	}
	float weight;
};
typedef bgl::adjacency_list<
	boost::listS, bgl::vecS, bgl::directedS,
	Importance,   Probability>
Graph;
//
// ///////////////////////////////////////////////////////////////////////////



// Descritpors
typedef bgl::graph_traits<Graph>::vertex_descriptor  VerDes;
typedef bgl::graph_traits<Graph>::edge_descriptor   EdgeDes;
// Iterators
typedef bgl::graph_traits<Graph>::vertex_iterator  VerIter;
typedef bgl::graph_traits<Graph>::edge_iterator   EdgeIter;
// Vertices maps
typedef bgl::property_map<Graph,bgl::vertex_index_t>::type   VertIndexMap;
typedef bgl::property_map<Graph,bgl::vertex_bundle_t>::type  VertPropMap;



int main(int, char**)
{
	// Graph object, so far unpopulated
	Graph G;

	// Vertices properties
	Importance nodesImportance[] = {
		Pair("Juan", 0.0),
		Pair("Pepe", 0.0),
		Pair(  "Ro", 0.0),
		Pair( "Ita", 0.0),
		Pair("RARE", 1.0)
	};
	const int numNodes(sizeof(nodesImportance)/sizeof(nodesImportance[0]));
	// Populate the graph with them
	for (int i=0 ; i<numNodes ; i++)
		bgl::add_vertex(nodesImportance[i], G);
	// Show them
	std::cout << "Num vertices: " << bgl::num_vertices(G) << std::endl;
	std::cout << "Vertices:" << std::endl;
	VertIndexMap viMap = bgl::get(bgl::vertex_index,  G);
	VertPropMap  vpMap = bgl::get(bgl::vertex_bundle, G);
	VerIter vit, vit_end;
	for (bgl::tie(vit,vit_end) = bgl::vertices(G); vit != vit_end ; vit++) {
		std::cout << "  "
				  << "[" << viMap[*vit] << "]: "
				  << "(" << vpMap[*vit].name
				  << "," << vpMap[*vit].importance << ")"
				  << std::endl;
	}
	std::cout << std::endl;

	// Edges properties
	typedef std::tuple<int,int,Probability> Edge;
	const std::vector<Edge> edges = {
		std::make_tuple(viMap[0], viMap[2], 1.0),
		std::make_tuple(viMap[1], viMap[1], 0.3),
		std::make_tuple(viMap[1], viMap[3], 0.5),
		std::make_tuple(viMap[1], viMap[4], 0.2),
		std::make_tuple(viMap[2], viMap[1], 0.7),
		std::make_tuple(viMap[2], viMap[3], 0.3),
		std::make_tuple(viMap[3], viMap[4], 1.0),
		std::make_tuple(viMap[4], viMap[0], 0.5),
		std::make_tuple(viMap[4], viMap[1], 0.5)
	};
	// Populate graph with them
	for (auto it = edges.begin() ; it != edges.end() ; it++) {
		int s, t;
		Probability p;
		std::tie(s, t, p) = *it;
		bgl::add_edge(s, t, p, G);
	}
	// Show them
	std::cout << "Num edges:" << bgl::num_edges(G) << std::endl;
	std::cout << "Edges:" << std::endl;
	EdgeIter eit, eit_end;
	for (bgl::tie(eit, eit_end) = bgl::edges(G) ; eit != eit_end ; eit++) {
		// For bgl::adjacency_list there is no indexed access of edges
		// (see http://goo.gl/Xdq9If) We use iterators for that.
		std::cout << " "
				  << "(" << bgl::source(*eit,G)
				  << "," << bgl::target(*eit,G) << "): "
				  << G[*eit].weight   // BLACK MAGIC: http://goo.gl/VC16UB
				  << std::endl;
	}
	std::cout << std::endl;

	// Find distance of each vertex to "RARE"
	vit = bgl::vertices(G).first;
	std::advance(vit, bgl::num_vertices(G)-1);
	VerDes rare = *vit;
	std::vector<int> distance(bgl::num_vertices(G));

//	TODO study "Extending algorithms with visitors" from the BGL tuto

	//	bgl::dijkstra_shortest_paths(G, rare, bgl::distance_map(&distance[0]));
	std::cout << "Distance to rare vertex:" << std::endl;
	viMap = bgl::get(bgl::vertex_index, G);
	for (bgl::tie(vit, vit_end) = bgl::vertices(G) ; vit != vit_end ; vit++) {
		std::cout << " "
				  << viMap[*vit]    << " --> "
				  << G[rare].name   << " : ";
//				  << distance[*vit] << std::endl;
	}

	return EXIT_SUCCESS;
}


