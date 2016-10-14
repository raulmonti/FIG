/* Leonardo Rodríguez */
#ifndef GRAPH_H
#define GRAPH_H

#include <functional>
#include <map>

/** @brief An implementation of a Graph using a multimap
 * from vertex to edges.
 */

namespace iosa {

/** @brief An edge of the graph: source, destination, and edge-data (e.g weight) */
template<typename V, typename D>
class Edge {
private:
    /// Source vertex
    V src;
    /// Destination vertex.
    V dst;
    /// Data of the edge (weight of the edge, or in our case, label of the transition)
    D data;
public:
    Edge(const V& src, const V& dst, const D& data) :
        src {src}, dst {dst}, data {data} {}

public: /// Accessors
    V get_src() const {
        return src;
    }

    V get_dst() const {
        return dst;
    }

    D get_data() const {
        return data;
    }
};

/**
 * Graph template.
 */
template<typename V, // Vertex type
         typename D, // Edge data type
         typename VComp = std::less<V>, // Vertex order
         typename DEq = std::equal_to<D>> // Data comparison
class Graph {
protected:
    // Maybe we should use unordered map and hash the vector
    // http://stackoverflow.com/questions/20511347/
    // http://stackoverflow.com/questions/37007307/

    /// Multimap from vertex to edges.
    std::multimap<V, Edge<V, D>, VComp> edges;

    /// Edge comparison
    VComp less = edges.key_comp();

    /// Data equality
    DEq data_eq = DEq();

public:
    Graph() {}

    /// Adds and edge to the graph.
    void add_edge(const Edge<V,D> &edge);

    /// Has this graph the given vertex?
    /// @note two vertices v and v' are considered equivalent
    /// if !less(v,v') and !less(v',v)
    bool has_vertex(const V& v) const;

    /// Has this graph the given edge?
    /// @note two edges are considered equal if they have
    /// the same source and destination vertices, and if they
    /// also have the same data.
    bool has_edge(const Edge<V,D> &edge) const;

    /// Are the given vertices equivalent using this class comparison object?
    bool same_vertex(const V& v1, const V& v2) const;

    /// Are the given edges equivalent using this class comparison object?
    bool same_edge(const Edge<V,D> &edge1, const Edge<V,D> &edge2) const;

    /// Prints debug information.
    void print(std::function<void (Edge<V, D>)> printer) const;
};

template<typename V, typename D, typename VComp, typename DEq>
void Graph<V,D,VComp,DEq>::add_edge(const Edge<V,D> &edge) {
    const auto& e = std::make_pair(edge.get_src(), edge);
    edges.insert(e);
}

template<typename V, typename D, typename VComp, typename DEq>
bool Graph<V,D,VComp,DEq>::has_vertex(const V& v) const {
    return edges.find(v) != edges.cend();
}

template<typename V, typename D, typename VComp, typename DEq>
bool Graph<V,D,VComp,DEq>::has_edge(const Edge<V,D> &edge) const {
    auto its = edges.equal_range(edge.get_src());
    auto fst = its.first;
    auto snd = its.second;
    bool result = false;
    while (fst != snd && !result) {
        const Edge<V,D>& curr = fst->second;
        result = same_edge(curr, edge);
        fst++;
    }
    return (result);
}

template<typename V, typename D, typename VComp, typename DEq>
bool Graph<V,D,VComp,DEq>::same_vertex(const V& v1, const V& v2) const {
    return (!less(v1, v2) && !less(v2, v1));
}

template<typename V, typename D, typename VComp, typename DEq>
bool Graph<V,D,VComp, DEq>::same_edge(const Edge<V,D> &edge1,
                                 const Edge<V,D> &edge2) const {
    return same_vertex(edge1.get_src(), edge2.get_src()) &&
            same_vertex(edge1.get_dst(), edge2.get_dst()) &&
            data_eq(edge1.get_data(), edge2.get_data());
}

template<typename V, typename D, typename VComp, typename DEq>
void Graph<V,D,VComp, DEq>::
print(std::function<void (Edge<V, D>)> printer) const {
    auto it = edges.cbegin();
    while (it != edges.cend()) {
        const Edge<V,D>&edge = it->second;
        printer(edge);
        it++;
    }
}

} //namespace iosa

#endif //GRAPH_H
