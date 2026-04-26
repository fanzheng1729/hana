#ifndef SIMPTREE_H_INCLUDED
#define SIMPTREE_H_INCLUDED

#include <cstddef>      // for NULL
#include <functional>   // for std::less
#include <vector>
#include "for.h"
#include "../util/algo.h"   // for util::addordered

// #undef __cpp_lib_incomplete_container_elements

namespace util
{
template<class T>
class SimpTree
{
public:
    typedef std::size_t size_type;
    // Wrapper of pointer to a node
    class pNode;
    // Node of the tree
    class TreeNode;
    // Children of a node
#ifdef __cpp_lib_incomplete_container_elements
    typedef std::vector<TreeNode> Children;
#else
    typedef std::vector<pNode> Children;
#endif // __cpp_lib_incomplete_container_elements
    // Node of the tree
    class TreeNode
    {
        friend pNode;
        friend SimpTree;
        // Vector of children
        Children children;
        // The value
        T value;
        TreeNode(T const & x) : value(x) {}
        friend bool operator==(T const & x, TreeNode const & y)
        { return x == y.value; }
        friend bool operator==(TreeNode const & x, T const & y)
        { return x.value == y; }
        friend bool operator!=(T const & x, TreeNode const & y)
        { return !(x == y); }
        friend bool operator!=(TreeNode const & x, T const & y)
        { return !(x == y); }
        friend bool operator< (T const & x, TreeNode const & y)
        { return x < y.value; }
        friend bool operator< (TreeNode const & x, T const & y)
        { return x.value == y; }
    }; // class TreeNode
private:
    // Pointer to the root
    TreeNode * m_data;
public:
    // Wrapper of pointer to a node
    class pNode
    {
        friend SimpTree;
        TreeNode * m_ptr;
        pNode(TreeNode * p) : m_ptr(p) {}
#ifndef __cpp_lib_incomplete_container_elements
        // Clear the node.
        // DO NOTHING if (!*this).
        void m_clear() const
        {
            if (!*this) return;
            FOR (pNode p, m_ptr->children) p.m_clear();
            delete m_ptr;
        }
#endif // __cpp_lib_incomplete_container_elements
        // Add a child. Return the pointer to the child.
        // *this should != nullptr.
        pNode m_insert(T const & t) const
        {
            reserve(m_ptr->children.size() + 1);
#ifdef __cpp_lib_incomplete_container_elements
            // Update the parent.
            m_ptr->children.push_back(t);
            // Pointer to the child.
            TreeNode * child = &m_ptr->children.back();
#else
            // Pointer to the child.
            TreeNode * child = new TreeNode(t);
            // Update the parent.
            m_ptr->children.push_back(child);
#endif // __cpp_lib_incomplete_container_elements

            return child;
        }
#ifndef __cpp_lib_incomplete_container_elements
        // Add a subtree with copying. Return pointer to the child.
        // *this should != nullptr.
        pNode m_insert(TreeNode const & node) const
        {
            // Add the root of the subtree.
            pNode child = m_insert(node.value);
            // Add all its children.
            child.m_insertchildren(node);

            return child;
        }
        // Add all children of a node.
        // *this should != nullptr.
        void m_insertchildren(TreeNode const & node) const
        {
            FOR (pNode p, node.children) m_insert(*p.m_ptr);
        }
#endif // __cpp_lib_incomplete_container_elements
    public:
        pNode() : m_ptr() {}
        pNode(TreeNode const & node) : m_ptr(const_cast<TreeNode *>(&node)) {}
        operator TreeNode const * () const { return m_ptr; }
        // Return true if a node has a child. Return 0 if *this is nullptr.
        bool haschild() const { return *this && !m_ptr->children.empty(); }
        // Return # children of a node. Return 0 if *this is nullptr.
        size_type nchild() const { return *this ? m_ptr->children.size() : 0; }
        // Return the content of a node. *this != nullptr.
        T & operator*() const { return m_ptr->value; }
        T * operator->()const { return&m_ptr->value; }
        friend bool operator==(pNode x, pNode y){ return *x == *y; }
        friend bool operator!=(pNode x, pNode y){ return *x != *y; }
        friend bool operator< (pNode x, pNode y){ return *x < *y; }
        // Return pointer to the children of a node.
        // Return nullptr if *this is nullptr.
        Children const * children() const
        { return *this ? &m_ptr->children : NULL; }
        // Reserve space for children and fix the parents of existing ones.
        void reserve(size_type n) const
        { if (*this) m_ptr->children.reserve(n); }
        // Add a child. Return the pointer to the child.
        // DO NOTHING and return nullptr if *this is nullptr.
        pNode insert(T const & t) const
        { return *this ? m_insert(t) : pNode(); }
        pNode insertordered(T const & t) const
        {
            if (!*this) return pNode;
            reserve(m_ptr->children.size() + 1);
#ifdef __cpp_lib_incomplete_container_elements
            // Pointer to the child.
            TreeNode * child = &*util::addordered(m_ptr->children, t);
#else
            // Pointer to the child.
            TreeNode * child = new TreeNode(t);
            // Update the parent.
            util::addordered(m_ptr->children, child);
#endif // __cpp_lib_incomplete_container_elements

            return child;
        }
    };
public:
    // Construct an empty tree.
    SimpTree() : m_data() {}
    // Construct a tree with 1 node.
    SimpTree(T const & value) : m_data(new TreeNode(value)) { m_data->size = 1; }
    // Copy CTOR
    SimpTree(SimpTree const & other) : m_data()
    {
        if (TreeNode const * p = other.m_data)
        {
            // Root node
#ifdef __cpp_lib_incomplete_container_elements
            m_data = new TreeNode(*p);
#else
            m_data = new TreeNode(p->value);
            // Add children.
            data().m_insertchildren(*p);
#endif // __cpp_lib_incomplete_container_elements
        }
    }
    // Move CTOR
    // Copy =
    SimpTree & operator=(SimpTree const & other)
    {
        if (data() == other.data())
            return *this;
        clear();
        return *(new(this) SimpTree(other));
    }
#if __cplusplus >= 201103L
    SimpTree(SimpTree && other) : m_data(other.m_data)
    { other.m_data = NULL; }
    // Move =
    SimpTree & operator=(SimpTree && other)
    {
        if (data() == other.data())
            return *this;
        clear();
        m_data = other.m_data;
        other.m_data = NULL;
    }
#endif // __cplusplus >= 201103L
    // Clear the tree.
#ifdef __cpp_lib_incomplete_container_elements
    void clear() { delete m_data; m_data = NULL; }
    ~SimpTree() { delete m_data; }
#else
    void clear() { data().m_clear(); m_data = NULL; }
    ~SimpTree() { data().m_clear(); }
#endif // __cpp_lib_incomplete_container_elements
    // The root node
    pNode data() const { return m_data; }
    pNode root() { return m_data;}
    pNode const root() const { return m_data; }
    // Return true if the tree is empty.
    bool empty() const { return !m_data; }
    // Add a child. Return the pointer to the child.
    // DO NOTHING and return nullptr if p is nullptr.
    pNode insert(pNode p, T const & value)
    {
        if (!p) return pNode();
        return p.m_insert(value);
    }
}; // class SimpTree
} // namespase util

#endif // SIMPTREE_H_INCLUDED
