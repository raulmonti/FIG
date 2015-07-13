#include <iostream>    // std::cout
#include <string>      // std::string
#include <vector>      // std::vector<>
#include <tuple>       // std::tuple<>
#include <limits>      // std::numeric_limits<>
#include <utility>     // std::pair
#include <algorithm>   // std::for_each, std::swap
#include <cassert>     // assert()
#include <cstring>     // strdup()
#include <ctime>       // clock, CLOCKS_PER_SEC
#include <unistd.h>    // EXIT_{SUCCESS,FAILURE}
#include <libgen.h>    // basename()
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_utility.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/transpose_graph.hpp>
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

struct GProbability
{
	GProbability() : weight(0.0) {}
	GProbability(float _weight) : weight(_weight) {
		if (MIN > weight || weight > MAX) {
			std::stringstream err;
			err << "general probability weights ∈ [" << MIN << "," << MAX << "] ";
			err << "but \"" << weight << "\" was given.";
			throw GraphException(err.str().c_str());
		}
	}
	float weight;
	static constexpr float MIN = -1.0;
	static constexpr float MAX =  1.0;
};

struct Probability : public GProbability
{
	Probability()              : GProbability()        {}
	Probability(float _weight) : GProbability(_weight) {
		if (MIN >= weight) {
			std::stringstream err;
			err << "general probability weights ∈ (" << MIN << "," << MAX << "] ";
			err << "but \"" << weight << "\" was given.";
			throw GraphException(err.str().c_str());
		}
	}
	Probability(GProbability&& that) : Probability(that.weight) {}
	Probability& operator=(GProbability that) {
		if (MIN >= that.weight || that.weight > MAX) {
			std::stringstream err;
			err << "probability weights ∈ (" << MIN << "," << MAX << "] ";
			err << "but passed general probability has weight " ;
			err << "\"" << weight << "\".";
			throw GraphException(err.str().c_str());
		}
		std::swap(weight, that.weight);
		return *this;
	}
	static constexpr float MIN = 0.0;
	static constexpr float MAX = 1.0;
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
	GProbability>  AdjGraph;

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




//  Auxiliary functions  //////////////////////////////////////////////////////

void TODO(int lineNum)
{
	std::stringstream TODO;
	TODO << basename(strdup(__FILE__ ":")) << lineNum << " -- TODO";
	throw GraphException(TODO.str().c_str());
}


void populateAdjacencyGraph(AdjGraph& g)
{
	if (bgl::num_vertices(g) > 0)
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
		bgl::add_vertex(states[i], g);
	}

	// Edges with properties
	typedef std::tuple<int,int,Probability> Edge;
	auto viMap = bgl::get(bgl::vertex_index, g);
	const std::vector<Edge> edges = {
		std::make_tuple(viMap[0], viMap[2], 1.0),
		std::make_tuple(viMap[1], viMap[1], std::numeric_limits<float>::min()),
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
		bgl::add_edge(s, t, p, g);
	}
}


/**
 * @brief Create new graph equal to argument but with edges reversed
 * @remarks Memory  heavy
 * @remarks Runtime light
 */
AdjGraph createTransposedGraph(const AdjGraph& g)
{
	AdjGraph gt;
	bgl::transpose_graph<AdjGraph, AdjGraph>(g, gt);
	assert(bgl::num_vertices(g) == bgl::num_vertices(gt));
	assert(bgl::num_edges(g) == bgl::num_edges(gt));
	return gt;  // You better be using move semantics, clang/gcc.
}


/**
 * @brief Reverse all edges from argument
 * @remarks Memory  medium
 * @remarks Runtime heavy
 */
void transposeGraph(AdjGraph& g)
{
	// Mirror all edges keeping track of freshness
//	bgl::graph_traits<AdjGraph>::edge_iterator eit, eit_end;
//	for (bgl::tie(eit, eit_end) = bgl::edges(g) ; eit != eit_end ; eit++) {
//		auto e = bgl::graph_traits<AdjGraph>::edge_descriptor(*eit);
	BGL_FORALL_EDGES(e, g, AdjGraph) {
		float pweight = g[e].weight;
		if (0.0 < pweight) {
			GProbability gp(-1.0*pweight);
			bgl::add_edge(bgl::target(e,g), bgl::source(e,g), gp, g);
			// In spite of what is stated here http://goo.gl/Uo1Ohb,
			// removing edges invalidates the loop iterator
			// regardless of the chosen EdgeList container type.
//			bgl::remove_edge(e, g);
		}
	}

	// Remove original edges
	struct check_for_old_edges {
		const AdjGraph& _g_;
		check_for_old_edges(const AdjGraph& _g) : _g_(_g) {}
		typedef bgl::graph_traits<AdjGraph>::edge_descriptor EdgeDescriptor;
		bool operator() (const EdgeDescriptor& e) const { return _g_[e].weight > 0.0; }
	} is_old_edge(g);
	bgl::remove_edge_if(is_old_edge, g);

	// Leave graph in valid state
	BGL_FORALL_EDGES(e, g, AdjGraph) {
		g[e].weight *= -1.0;
	}
}


CSRGraph crystallizeGraph(const AdjGraph& g)
{
	TODO(__LINE__);
	return CSRGraph();
}



//  Main program  /////////////////////////////////////////////////////////////

int main (int, char**)
{
	float t;
	AdjGraph model;

	cout << endl << "  - Create some random mutable graph" << endl << endl;
	populateAdjacencyGraph(model);
	bgl::print_graph(model);

	cout << endl << "  - Reverse its edges ";
		t = static_cast<float>(clock());
	model = createTransposedGraph(model);
		cout << "(" << ((clock()-t)/CLOCKS_PER_SEC) << " s)" << endl << endl;
		bgl::print_graph(model);

	cout << endl << "  - Reverse again, obtaining original back ";
		t = static_cast<float>(clock());
	transposeGraph(model);
		cout << "(" << ((clock()-t)/CLOCKS_PER_SEC) << " s)" << endl << endl;
		bgl::print_graph(model);

	cout << endl << "  - Compact graph into immutable CSR format ";
		t = static_cast<float>(clock());
	CSRGraph finalModel = crystallizeGraph(model);
		cout << "(" << ((clock()-t)/CLOCKS_PER_SEC) << " s)" << endl << endl;
		bgl::print_graph(finalModel);

	// TODO
	//     Study BGL visitors?  http://www.boost.org/doc/libs/1_58_0/libs/graph/doc/DijkstraVisitor.html

	return EXIT_SUCCESS;
}
