#include <iostream>    // std::cout
#include <string>      // std::string
#include <utility>     // std::pair
#include <algorithm>   // std::for_each
#include <unistd.h>    // EXIT_{SUCCESS,FAILURE}
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>



typedef std::string str;
typedef std::pair<str,float> Pair;


// ///////////////////////////////////////////////////////////////////////////
//
//	NOTE: will use Boost bundled properties (http://goo.gl/mJS8gP)
//
//	typedef boost::property<boost::vertex_distance_t, float,
//			boost::property<boost::vertex_name_t, std::string>>
//		VertexProperty;
//	typedef boost::property<boost::edge_weight_t, float>
//		EdgeProperty;
struct Importance {
	Importance(const str& name_, const float& importance_) :
		name(name_),
		importance(importance_)
	{}
	Importance(const Pair& imp) :
		name(imp.first),
		importance(imp.second)
	{}
	str   name;
	float importance;
};
struct Probability {
	float weight;
};
//	typedef boost::adjacency_list<
//		boost::listS, boost::vecS, boost::directedS,
//		VertexProperty, EdgeProperty> Graph;
typedef boost::adjacency_list<
	boost::listS, boost::vecS, boost::directedS,
	Importance, Probability> Graph;
//
// ///////////////////////////////////////////////////////////////////////////


// Descritpors
typedef boost::graph_traits<Graph>::vertex_descriptor  VerDes;
typedef boost::graph_traits<Graph>::edge_descriptor    EdgDes;
// Iterators
typedef boost::graph_traits<Graph>::vertex_iterator  VerIter;
typedef boost::graph_traits<Graph>::edge_iterator    EdgIter;
// Vertices maps
typedef boost::property_map<Graph,boost::vertex_index_t>::type   VertIndexMap;
typedef boost::property_map<Graph,boost::vertex_bundle_t>::type  VertPropMap;
// Edges maps
//typedef boost::property_map<Graph,boost::edge_index_t>::type   EdgeIndexMap;
//typedef boost::property_map<Graph,boost::edge_bundle_t>::type  EdgePropMap;


int main(int, char**)
{

	// Graph object, so far unpopulated
	Graph G;

	// Vertices properties
	Importance nodesImportance[] = {
		Pair(   "0", 0.0),
		Pair(   "1", 0.0),
		Pair(   "2", 0.0),
		Pair(   "3", 0.0),
		Pair("RARE", 1.0)
	};
	const int numNodes(sizeof(nodesImportance)/sizeof(nodesImportance[0]));
	// Populate the graph with them
	for (int i=0 ; i<numNodes ; i++)
		boost::add_vertex(nodesImportance[i], G);
	// Show them
	std::cout << "Num vertices: " << boost::num_vertices(G) << std::endl;
	std::cout << "Vertices:" << std::endl;
	VertIndexMap viMap = boost::get(boost::vertex_index,  G);
	VertPropMap  vpMap = boost::get(boost::vertex_bundle, G);
	VerIter vit, vit_end;
	for (boost::tie(vit,vit_end) = boost::vertices(G); vit != vit_end ; vit++) {
		std::cout << "  "
				  << "[" << viMap[*vit] << "]: "
				  << "(" << vpMap[*vit].name
				  << "," << vpMap[*vit].importance << ")"
				  << std::endl;
	}
	std::cout << std::endl;

//	// Edges properties
//	typedef std::pair<int,int> Edge;
//	const Edge edges[numEdges] = {
//		Edge(vmap[0],vmap[2]),
//		Edge(vmap[1],vmap[1]), Edge(vmap[1],vmap[3]), Edge(vmap[1],vmap[4]),
//		Edge(vmap[2],vmap[1]), Edge(vmap[2],vmap[3]),
//		Edge(vmap[3],vmap[4]),
//		Edge(vmap[4],vmap[0]), Edge(vmap[4],vmap[1])
//	};
//	const int numEdges(sizeof(edges)/sizeof(edges[0]));
//	const Probability edgesWeight[numEdges] = {
//		1.0,
//		0.3, 0.5, 0.2,
//		0.7, 0.3,
//		1.0,
//		0.5, 0.5
//	};
//	// Graph
//	for (int j=0 ; j<numEdges ; j++)
//		boost::add_edge()
//	Graph G(edges,
//			edges + sizeof(edges) / sizeof(Edge),
//			nodesImportance,
//			edgesWeight,
//			boost::graph_property<Graph,>::type,
//			numNodes);

	return EXIT_SUCCESS;
}


