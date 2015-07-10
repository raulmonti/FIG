#include <iostream>    // std::cout
#include <string>      // std::string
#include <vector>      // std:::vector<>
#include <tuple>       // std::tuple<>
#include <utility>     // std::pair
#include <algorithm>   // std::for_each, std::swap
#include <unistd.h>    // EXIT_{SUCCESS,FAILURE}
#include <libgen.h>    // basename()
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/graph/compressed_sparse_row_graph.hpp>


namespace bgl = boost;
using std::string;
using std::vector;
using std::cout;
using std::endl;
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

struct State
{
	State() : name(""), importance(0) {}
	State(const string& _name, const float& _importance) :
		name(_name),
		importance(_importance)
	{}
	State(const Pair& imp) :
		name(imp.first),
		importance(imp.second)
	{}
	State(const State& that) :
		name(that.name),
		importance(that.importance)
	{}
	State(State&& that) noexcept :
		name(std::move(that.name)),
		importance(std::move(that.importance))
	{}
	State& operator=(State that) {
		std::swap(name, that.name);
		std::swap(importance, that.importance);
		return *this;
	}
	string name;
	float importance;
};

struct Probability
{
	Probability() : weight(0.0) {}
	Probability(float _weight) : weight(_weight) {
		if (0.0 > weight || weight > 1.0)
			throw GraphException("probability weights ∈ [0,1]");
	}
	float weight;
};

// template <class OutEdgeListS = vecS, // a Sequence or an AssociativeContainer
//           class VertexListS = vecS,  // a Sequence or a RandomAccessContainer
//           class DirectedS = directedS,
//           class VertexProperty = no_property,
//           class EdgeProperty = no_property,
//           class GraphProperty = no_property,
//           class EdgeListS = listS>
typedef bgl::adjacency_list<
	bgl::vecS,
	bgl::vecS,
	bgl::directedS,
	State,
	Probability>  AdjGraph;

// template<typename Directed = directedS,
//          typename VertexProperty = no_property,
//          typename EdgeProperty = no_property,
//          typename GraphProperty = no_property,
//          typename Vertex = std::size_t,
//          typename EdgeIndex = Vertex>
typedef bgl::compressed_sparse_row_graph<
	bgl::directedS,
	State,
	Probability>  CSRGraph;



//  Global variables  ////////////////////////////////////////////////////////

AdjGraph model;



//  Auxiliary functions  //////////////////////////////////////////////////////

void TODO(int lineNum)
{
	static const string file(basename(const_cast<char*>(string(__FILE__).c_str())));
	std::stringstream TODO; TODO << file << ":" << lineNum << " -- TODO";
	throw GraphException(TODO.str().c_str());
}

void populateAdjacencyGraph()
{
	if (bgl::num_vertices(model) > 0)
		return;  // already has something

	// Vertices with properties
	State states[] = {
		Pair("Juan", 0.0),
		Pair("Pepe", 0.0),
		Pair(  "Ro", 0.0),
		Pair( "Ita", 0.0),
		Pair("RARE", 1.0)
	};
	const int numNodes(sizeof(states)/sizeof(states[0]));
	// Feed them into the graph
	for (int i=0 ; i<numNodes ; i++) {
		bgl::add_vertex(states[i], model);
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


void reverseGraphEdges(AdjGraph& g)
{
	TODO(__LINE__);
}


CSRGraph crystallizeGraph(const AdjGraph& g)
{
	TODO(__LINE__);
	return CSRGraph();
}



//  Main program  /////////////////////////////////////////////////////////////

int main (int, char**)
{
	// Create some random mutable graph
	cout << endl << "  -  -  -  -  -  -  -  -  -  -" << endl << endl;
	populateAdjacencyGraph();
	bgl::print_graph(model);

	// Reverse its edges
	cout << endl << "  -  -  -  -  -  -  -  -  -  -" << endl << endl;
	reverseGraphEdges(model);
	bgl::print_graph(model);

	// Reverse edges again, result whould be as the original graph
	cout << endl << "  -  -  -  -  -  -  -  -  -  -" << endl << endl;
	reverseGraphEdges(model);
	bgl::print_graph(model);

	// Compact graph into CSR format
	cout << endl << "  -  -  -  -  -  -  -  -  -  -" << endl << endl;
	CSRGraph finalModel = crystallizeGraph(model);
	bgl::print_graph(finalModel);
	cout << endl << endl;

	// TODO
	//     Study BGL visitors?  http://www.boost.org/doc/libs/1_58_0/libs/graph/doc/DijkstraVisitor.html

	return EXIT_SUCCESS;
}
