#ifndef DAG_H_INCLUDED
#define DAG_H_INCLUDED

#include <algorithm>    // for std:.*_bound
#include <set>
#include <utility>
#include <vector>
#include "for.h"

// Directed acyclic graph, on top of a set
template<class Carrier>
struct DAG : Carrier
{
    typedef typename Carrier::const_iterator    const_iterator;
    typedef typename Carrier::iterator          iterator;
    typedef typename Carrier::value_type        value_type;
    typedef std::vector<const_iterator> iterators;
    typedef std::pair<const_iterator, const_iterator> Edge;
    struct EdgeComp
    {
        bool operator()(Edge x, Edge y) const
        {
            return *x.first < *y.first || *x.first == *y.first && *x.second < *y.second;
        }
    };
    typedef std::set<Edge, EdgeComp> Edges;
    void clear()
    {
        Carrier::clear();
        edges.clear(), redges.clear(), reaches.clear(), rreaches.clear();
        toporder.clear();
    }
    // Add an element.
    std::pair<iterator, bool> insert(const value_type & value)
    {
        std::pair<iterator, bool> result = Carrier::insert(value);
        if (result.second)
            toporder.push_back(result.first);
        return result;
    }
    // Add an edge. Returns true if edge is added.
    bool link(const_iterator from, const_iterator to)
    {
        if (from == to || linked(from, to) || reachable(to, from))
            return false; // Self loop or cycle not allowed
        // Update the topological sort.
        if (!reorder(from, to))
            return false;
        // Update reachability.
        addreaches(from, to);
        // Add the edge
        edges.insert(Edge(from, to));
        redges.insert(Edge(to, from));
        return true;
    }
    // Return true if an edge exists.
    bool linked(const_iterator from, const_iterator to) const
    {
        return edges.count(Edge(from, to));
    }
    // Return true if a node reaches the other.
    bool reachable(const_iterator from, const_iterator to) const
    {
        return reaches.count(Edge(from, to));
    }
    // Return the set of nodes reachable to a node.
    iterators reachesfrom(const_iterator to) const
    {
        Edge minedge(to, this->begin());
        Edge maxedge(to, --this->end());
        typename Edges::iterator begin
            = std::lower_bound(rreaches.begin(), rreaches.end(), minedge, EdgeComp());
        typename Edges::iterator end
            = std::upper_bound(rreaches.begin(), rreaches.end(), maxedge, EdgeComp());

        iterators result;
        // Preallocate for efficiency
        result.reserve(this->size() - 1);
        for ( ; begin != end; ++begin)
            result.push_back(begin->second);
        return result;
    }
    // Return the set of nodes reachable from a node.
    iterators reachesto(const_iterator from) const
    {
        Edge minedge(from, this->begin());
        Edge maxedge(from, --this->end());
        typename Edges::iterator begin
            = std::lower_bound(reaches.begin(), reaches.end(), minedge, EdgeComp());
        typename Edges::iterator end
            = std::upper_bound(reaches.begin(), reaches.end(), maxedge, EdgeComp());

        iterators result;
        // Preallocate for efficiency
        result.reserve(this->size() - 1);
        for ( ; begin != end; ++begin)
            result.push_back(begin->second);
        return result;
    }
private:
    Edges edges, redges, reaches, rreaches;
    iterators toporder;
    typedef typename iterators::iterator orderiter;
    // Reorder two nodes in the topological order.
    // from and to are iterators to distinct elements in the set.
    bool reorder(const_iterator from, const_iterator to)
    {
        orderiter fromiter = std::find(toporder.begin(), toporder.end(), from);
        if (fromiter == toporder.end())
            return false;
        orderiter toiter = std::find(toporder.begin(), toporder.end(), to);
        if (toiter == toporder.end())
            return false;
        if (fromiter < toiter)
            return true;
        // Nodes to be moved after from
        iterators tobemoved;
        // Preallocate for efficiency.
        tobemoved.reserve(fromiter - toiter);
        // The to node is to be moved after the from node.
        tobemoved.push_back(to);
        // Output to the new topological order
        orderiter out = toiter;
        for (++toiter; toiter < fromiter; ++toiter)
        {
            if (reachable(to, *toiter))
                // Nodes reachable from to are to be moved after the from node.
                tobemoved.push_back(*toiter);
            else
                // Nodes unreachable from to move forward to fill in the space.
                *out++ = *toiter;
        }
        // The from node
        *out++ = from;
        // Move nodes reachable from the to node after the from node.
        std::copy(tobemoved.begin(), tobemoved.end(), out);

        return true;
    }
    // Add reachability brought by the new edge.
    void addreaches(const_iterator from, const_iterator to)
    {
        if (reachable(from, to))
            return; // already reachable

        iterators src = reachesfrom(from);
        src.push_back(from);
        iterators dst = reachesto(to);
        dst.push_back(to);
        FOR (const_iterator iter, src)
            FOR (const_iterator iter2, dst)
            {
                reaches.insert(Edge(iter, iter2));
                rreaches.insert(Edge(iter2, iter));
            }
    }
};

template<class T> bool testDAG(T n)
{
    n = n ? n : 1;
    DAG<std::set<T> > dag;
    DAG<std::set<T> >::iterators v;
    v.reserve(2 * n);
    for (T i = 0; i < n; ++i)
        v.push_back(dag.insert(2 * i).first);
    for (T i = 0; i < n; ++i)
        v.push_back(dag.insert(2 * i + 1).first);
    for (T i = 0; i < n - 1; ++i)
        if (!dag.link(v[i], v[i + 1]))
            return false;
    for (T i = 0; i < n - 1; ++i)
        if (!dag.link(v[n + i], v[n + i + 1]))
            return false;
    if (!dag.link(v[n - 1], v[n]))
        return false;
    if (dag.link(v[2 * n - 1], v[0]))
        return false;
    for (T i = 0; i < 2 * n; ++i)
        for (T j(i + 1); j < 2 * n; ++j)
            if (!dag.reachable(v[i], v[j]) ||
                dag.reachable(v[j], v[i]))
                return false;
    for (T i = 0; i < 2 * n; ++i)
        if (dag.reachesfrom(v[i]).size() != i ||
            dag.reachesto(v[i]).size() != 2 * n - 1 - i)
            return false;

    return true;
}

#endif // DAG_H_INCLUDED
