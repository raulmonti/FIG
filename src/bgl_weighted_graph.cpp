#include <iostream>    // std::cout
#include <iomanip>     // std::fixed, std::setprecision
#include <string>      // std::string
#include <vector>      // std:::vector<>
#include <tuple>       // std::tuple<>
#include <utility>     // std::pair
#include <algorithm>   // std::for_each, std::swap
#include <unistd.h>    // EXIT_{SUCCESS,FAILURE}
#include <boost/tuple/tuple.hpp>
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
		return *this;
	}
	float weight;
};
typedef bgl::adjacency_list<
	bgl::listS, bgl::vecS, bgl::directedS,
	Importance, Probability>
Graph;
//
// ///////////////////////////////////////////////////////////////////////////



// Descriptors
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

	// Vertices with properties
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
	//VertIndexMap viMap = bgl::get(bgl::vertex_index,  G);
	//VertPropMap  vpMap = bgl::get(bgl::vertex_bundle, G);
	auto viMap = bgl::get(bgl::vertex_index,  G);
	auto vpMap = bgl::get(bgl::vertex_bundle, G);
	VerIter vit, vit_end;
	for (bgl::tie(vit,vit_end) = bgl::vertices(G); vit != vit_end ; vit++) {
		std::cout << "  "
				  << "[" << viMap[*vit] << "]: "  // Just "*vit"    also works
				  << "(" << vpMap[*vit].name      // "G[*vit].name" also works
				  << "," << vpMap[*vit].importance << ")"  // IDEM above
				  << std::endl;
	}
	std::cout << std::endl;

	// Edges with properties
	typedef std::tuple<int,int,Probability> Edge;
	const std::vector<Edge> edges = {
		std::make_tuple(viMap[0], viMap[2], 1.0),
		std::make_tuple(viMap[1], viMap[1], 0.0),
		std::make_tuple(viMap[1], viMap[3], 0.6),
		std::make_tuple(viMap[1], viMap[4], 0.4),
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
	std::cout << "Num edges: " << bgl::num_edges(G) << std::endl;
	std::cout << "Edges:" << std::endl;
	EdgeIter eit, eit_end;
	for (bgl::tie(eit, eit_end) = bgl::edges(G) ; eit != eit_end ; eit++) {
		// For boost::adjacency_list there is no indexed access of edges
		// (see http://goo.gl/Xdq9If) We index the Graph itself with eit,
		// instead of building an EdgeSomethingMap.
		std::cout << "  "
				  << *eit
				  << " = (" << std::fixed << std::setw(4)
				  << vpMap[bgl::source(*eit,G)].name
				  << ","    << std::fixed << std::setw(4)
				  << vpMap[bgl::target(*eit,G)].name
				  << "): "  << std::fixed << std::setprecision(1)
				  << G[*eit].weight  // BLACK MAGIC: http://goo.gl/VC16UB
				  << std::endl;
	}
	std::cout << std::endl;

	// Find distance from "RARE" to each vertex
	vit = bgl::vertices(G).first;
	std::advance(vit, bgl::num_vertices(G)-1);
	VerDes rare = *vit;
	std::vector<float> distance(bgl::num_vertices(G));
	// Map for dealing with bundled properties (http://goo.gl/mJS8gP)
	//auto weights = bgl::get(&Probability::weight, G);
	typedef bgl::property_map<Graph, float Probability::*>::type EdgeWeightMap;
	EdgeWeightMap weights = bgl::get(&Probability::weight, G);
	bgl::dijkstra_shortest_paths(G, rare, weight_map(weights).distance_map(
		bgl::make_iterator_vertex_map(distance.begin(), G)));
	std::cout << "Distance from rare vertex:" << std::endl;
	for (bgl::tie(vit, vit_end) = bgl::vertices(G) ; vit != vit_end ; vit++) {
		std::cout << "  "    << std::fixed << std::setw(4)
				  << G[rare].name
				  << " --> " << std::fixed << std::setw(4)
				  << G[*vit].name
				  << ": "    << std::fixed << std::setprecision(1)
				  << distance[*vit] << std::endl;
	}

	return EXIT_SUCCESS;
}

/*
 * Output:
 *
 * Num vertices: 5
 * Vertices:
 *   [0]: (Juan,0)
 *   [1]: (Pepe,0)
 *   [2]: (Ro,0)
 *   [3]: (Ita,0)
 *   [4]: (RARE,1)
 *
 * Num edges: 9
 * Edges:
 *   (0,2) = (Juan,  Ro): 1.0
 *   (1,1) = (Pepe,Pepe): 0.0
 *   (1,3) = (Pepe, Ita): 0.6
 *   (1,4) = (Pepe,RARE): 0.4
 *   (2,1) = (  Ro,Pepe): 0.7
 *   (2,3) = (  Ro, Ita): 0.3
 *   (3,4) = ( Ita,RARE): 1.0
 *   (4,0) = (RARE,Juan): 0.5
 *   (4,1) = (RARE,Pepe): 0.5
 *
 * Distance from rare vertex:
 *   RARE --> Juan: 0.5
 *   RARE --> Pepe: 0.5
 *   RARE -->   Ro: 1.5
 *   RARE -->  Ita: 1.1
 *   RARE --> RARE: 0.0
 *
 */

