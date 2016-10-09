/* Leonardo Rodríguez */
#ifndef GRAPH_H
#define GRAPH_H

#include <functional>
#include <map>

namespace iosa {

template<typename V, typename D>
class Edge {
private:
    V src;
    V dst;
    D data;
public:
    Edge(const V& src, const V& dst, const D& data) :
        src {src}, dst {dst}, data {data} {}

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

template<typename V, typename D, typename VComp = std::less<V>>
class Graph {
protected:
    // Maybe we should use unordered map and hash the vector
    // http://stackoverflow.com/questions/20511347/
    // http://stackoverflow.com/questions/37007307/

    std::multimap<V, Edge<V, D>, VComp> edges;
    VComp less = edges.key_comp();
public:
    Graph() {}

    void add_edge(const Edge<V,D> &edge);

    bool has_vertex(const V& v) const;

    bool has_edge(const Edge<V,D> &edge) const;

    bool same_vertex(const V& v1, const V& v2) const;

    bool same_edge(const Edge<V,D> &edge1, const Edge<V,D> &edge2) const;

    void print(std::function<void (Edge<V, D>)> printer) const;
};

template<typename V, typename D, typename VComp>
void Graph<V,D,VComp>::add_edge(const Edge<V,D> &edge) {
    const auto& e = std::make_pair(edge.get_src(), edge);
    edges.insert(e);
}

template<typename V, typename D, typename VComp>
bool Graph<V,D,VComp>::has_vertex(const V& v) const {
    return edges.find(v) != edges.cend();
}

template<typename V, typename D, typename VComp>
bool Graph<V,D,VComp>::has_edge(const Edge<V,D> &edge) const {
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

template<typename V, typename D, typename VComp>
bool Graph<V,D,VComp>::same_vertex(const V& v1, const V& v2) const {
    return (!less(v1, v2) && !less(v2, v1));
}

template<typename V, typename D, typename VComp>
bool Graph<V,D,VComp>::same_edge(const Edge<V,D> &edge1,
                                 const Edge<V,D> &edge2) const {
    return same_vertex(edge1.get_src(), edge2.get_src()) &&
            same_vertex(edge1.get_dst(), edge2.get_dst());
}

template<typename V, typename D, typename VComp>
void Graph<V,D,VComp>::print(std::function<void (Edge<V, D>)> printer) const {
    auto it = edges.cbegin();
    while (it != edges.cend()) {
        const Edge<V,D>&edge = it->second;
        printer(edge);
        it++;
    }
}

} //namespace iosa

#endif //GRAPH_H
