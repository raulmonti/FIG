#include <iostream>    // std::cout
#include <string>      // std::string
#include <vector>      // std:::vector<>
#include <tuple>       // std::tuple<>
#include <utility>     // std::pair
#include <algorithm>   // std::for_each, std::swap
#include <unistd.h>    // EXIT_{SUCCESS,FAILURE}
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/compressed_sparse_row_graph.hpp>


namespace bgl = boost;
using std::string;
using std::vector;
typedef std::pair<string,float> Pair;



//  Global types  ////////////////////////////////////////////////////////////

class GraphException : public std::exception
{
	string msg;
	virtual const char* what() const throw() {
		return string("Exception raised: ").append(msg).c_str();
	}
public:
	GraphException(const char* _msg) : msg(_msg) {}
};

struct Importance
{
	Importance() : name(""), importance(0) {}
	Importance(const string& _name, const float& _importance) :
		name(_name),
		importance(_importance)
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
	Probability(float _weight) : weight(_weight) {
		if (0.0 > weight || weight > 1.0)
			throw GraphException("probability weights ∈ [0,1]");
	}
	Probability& operator=(Probability that) {
		std::swap(weight, that.weight);
		return *this;
	}
	float weight;
};

typedef bgl::adjacency_list<bgl::vecS,  bgl::vecS, bgl::directedS,
							Importance, Probability>
	AdjGraph;

typedef bgl::compressed_sparse_row_graph<> CSRGraph;



//  Global variables  ////////////////////////////////////////////////////////

AdjGraph model;



//  Auxiliary functions  //////////////////////////////////////////////////////

void populateAdjacencyGraph()
{
	if (bgl::num_vertices(model) > 0)
		return;  // already has something

	// Vertices with properties
	Importance nodesImportance[] = {
		Pair("Juan", 0.0),
		Pair("Pepe", 0.0),
		Pair(  "Ro", 0.0),
		Pair( "Ita", 0.0),
		Pair("RARE", 1.0)
	};
	const int numNodes(sizeof(nodesImportance)/sizeof(nodesImportance[0]));
	// Feed them into the graph
	for (int i=0 ; i<numNodes ; i++) {
		bgl::add_vertex(nodesImportance[i], model);
	}

	// Edges with properties
	typedef std::tuple<int,int,Probability> Edge;
	auto viMap = bgl::get(bgl::vertex_index, model);
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
	// Feed them into the graph
	for (auto it = edges.begin() ; it != edges.end() ; it++) {
		int s, t;
		Probability p;
		std::tie(s, t, p) = *it;
		bgl::add_edge(s, t, p, model);
	}
}



//  Main program  /////////////////////////////////////////////////////////////

int main (int, char**)
{

	populateAdjacencyGraph();

	bgl::print_graph(model);

	// TODO
	//     Create adjacency list graph, reverse edges and transform into CSR graph
	//     Print both structures and compare (check boost::print_graph)
	//     Study BGL visitors?  http://www.boost.org/doc/libs/1_58_0/libs/graph/doc/DijkstraVisitor.html

	return EXIT_SUCCESS;
}
