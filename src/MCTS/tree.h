#ifndef TREE_H_INCLUDED
#define TREE_H_INCLUDED

#include <cstddef>      // for std::size_t and NULL
#include <functional>   // for std::less
#include <vector>
#include "../util/for.h"
// #undef __cpp_lib_incomplete_container_elements

#ifdef __cpp_lib_incomplete_container_elements
enum NodeMovement {NONEMOVED = 0, SELFMOVED, CHILDMOVED};
#endif // __cpp_lib_incomplete_container_elements

template<class T>
class Tree
{
public:
    typedef std::size_t size_type;
    // Wrapper of pointer to a node
    class Nodeptr;
    // Node of the tree
    class TreeNode;
    // Children of a node
#ifdef __cpp_lib_incomplete_container_elements
    typedef std::vector<TreeNode> Children;
#else
    typedef std::vector<Nodeptr> Children;
#endif // __cpp_lib_incomplete_container_elements
    // Node of the tree
    class TreeNode
    {
        friend Nodeptr;
        friend Tree;
        // Pointer to the parent
        TreeNode * parent;
        // Size of the subtree, at least 1
        size_type size;
        // Vector of children
        Children children;
        // The value
        T value;
        // Constructor. DOES NOT set size
        TreeNode(T const & x) : parent(NULL), value(x) {}
        // Change the sizes of the node and its ancestors.
        void incsize(size_type const n)
        {
            for (TreeNode * p = this; p; p = p->parent)
                p->size += n;
        }
#ifdef __cpp_lib_incomplete_container_elements
        // Fix parent pointers of children.
        // Return the movement of children and itself.
        NodeMovement fixparents()
        {
            bool selfmoved = false;
            // Check if the node itself is moved.
            // If so, fix its children's parent pointer.
            FOR (TreeNode & child, children)
                if (child.parent != this)
                {
                    child.parent = this;
                    selfmoved = true;
                }
            // Node itself is not moved.
            if (!selfmoved)
                return NONEMOVED;
            // Node itself is moved. Check if its children moved.
            bool childmoved = false;
            FOR (TreeNode & child, children)
                childmoved |= (child.fixparents() != NONEMOVED);
            return childmoved ? CHILDMOVED : SELFMOVED;
        }
#endif // __cpp_lib_incomplete_container_elements
    }; // class TreeNode
private:
    // Pointer to the root
    TreeNode * m_data;
public:
    // Wrapper of pointer to a node
    class Nodeptr
    {
        friend Tree;
        TreeNode * m_ptr;
        Nodeptr(TreeNode * p) : m_ptr(p) {}
#ifndef __cpp_lib_incomplete_container_elements
        // Clear the node.
        // DO NOTHING if (!*this).
        void m_clear() const
        {
            if (!*this) return;
            FOR (Nodeptr p, m_ptr->children) p.m_clear();
            delete m_ptr;
        }
#endif // __cpp_lib_incomplete_container_elements
        // Add a child. Return the pointer to the child.
        // DOES NOT set the size. *this should != NULL.
        Nodeptr m_insert(T const & t) const
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
            // Update the child.
            child->parent = m_ptr;

            return child;
        }
#ifndef __cpp_lib_incomplete_container_elements
        // Add a subtree with copying. Return pointer to the child.
        // *this should != NULL.
        Nodeptr m_insert(TreeNode const & node) const
        {
            // Add the root of the subtree.
            Nodeptr child = m_insert(node.value);
            child.m_ptr->size = node.size;
            // Add all its children.
            child.m_insertchildren(node);

            return child;
        }
        // Add all children of a node.
        // *this should != NULL.
        void m_insertchildren(TreeNode const & node) const
        {
            FOR (Nodeptr p, node.children) m_insert(*p.m_ptr);
        }
#endif // __cpp_lib_incomplete_container_elements
    public:
        Nodeptr() : m_ptr(NULL) {}
        Nodeptr(TreeNode const & node) : m_ptr(const_cast<TreeNode *>(&node)) {}
        operator TreeNode const * () const { return m_ptr; }
        friend inline bool operator<(Nodeptr x, Nodeptr y)
            { return std::less<TreeNode *>(x, y); }
        // Return the parent. Return NULL if *this is NULL.
        Nodeptr parent() const { return Nodeptr(*this ? m_ptr->parent : NULL); }
        // Return the size. Return 0 if *this is NULL.
        size_type size() const { return *this ? m_ptr->size : 0; }
        // Return true if a node has grand child. Return 0 if *this is NULL.
        bool hasgrandchild() const
            { return *this && m_ptr->size > m_ptr->children.size() + 1; }
        // Return the content of a node. *this != NULL.
        T & operator*() const { return m_ptr->value; }
        T * operator->()const { return&m_ptr->value; }
        // Return pointer to the children of a node.
        // Return NULL if *this is NULL.
        Children const * children() const
            { return *this ? &m_ptr->children : NULL; }
        // Return pointer to a child of a node.
        // Return NULL if *this is NULL.
        Nodeptr child(size_type index) const
            { return *this ? m_ptr->children[index] : Nodeptr(); }
        // Reserve space for children and fix the parents of existing ones.
        // Return the movement of children and grandchildren.
        NodeMovement reserve(size_type n) const
        {
            if (!*this)
                return NONEMOVED;
            bool const childmoved = !m_ptr->children.empty() &&
                                    n > m_ptr->children.capacity();
            m_ptr->children.reserve(n);
#ifdef __cpp_lib_incomplete_container_elements
            if (!childmoved)
                return NONEMOVED;
            if (!hasgrandchild())
                return SELFMOVED;
            // (childmoved && hasgrandchild())
            bool grandchildmoved = false;
            FOR (Nodeptr child, m_ptr->children)
                grandchildmoved |=
                (child.m_ptr->fixparents() == CHILDMOVED);
            return grandchildmoved ? CHILDMOVED : SELFMOVED;
#endif // __cpp_lib_incomplete_container_elements
            return NONEMOVED;
        }
        // Return true if *this is ancestor of p.
        // Return false if p is NULL
        bool isancestorof(Nodeptr p) const
        {
            for (p = p.parent(); p; p = p.parent())
                if (this->m_ptr == p.m_ptr)
                    return true;
            return false;
        }
    }; // class Nodeptr
public:
    // Construct an empty tree.
    Tree() : m_data(NULL) {}
    // Construct a tree with 1 node.
    Tree(T const & value) : m_data(new TreeNode(value)) { m_data->size = 1; }
    // Copy CTOR
    Tree(Tree const & other) : m_data(NULL)
    {
        if (TreeNode const * p = other.m_data)
        {
            // Root node
#ifdef __cpp_lib_incomplete_container_elements
            m_data = new TreeNode(*p);
            // Fix parent pointers of children.
            m_data->fixparents();
#else
            m_data = new TreeNode(p->value);
            m_data->size = p->size;
            // Add children.
            data().m_insertchildren(*p);
#endif // __cpp_lib_incomplete_container_elements
        }
    }
    // Copy =
    Tree & operator=(Tree const & other)
    {
        if (data() == other.data())
            return *this;
#ifdef __cpp_lib_incomplete_container_elements
        delete m_data;
#else
        data().m_clear();
#endif // __cpp_lib_incomplete_container_elements
        return *(new(this) Tree(other.data()));
    }
    // Clear the tree.
#ifdef __cpp_lib_incomplete_container_elements
    void clear() { delete m_data; m_data = NULL; }
    ~Tree() { delete m_data; }
#else
    void clear() { data().m_clear(); m_data = NULL; }
    ~Tree() { data().m_clear(); }
#endif // __cpp_lib_incomplete_container_elements
    // The root node
    Nodeptr data() const { return m_data; }
    Nodeptr root() { return m_data;}
    Nodeptr const root() const { return m_data; }
    // Return true if the tree is empty.
    bool empty() const { return !m_data; }
    // Return size of the tree.
    size_type size() const {return m_data->size; }
    // Add a child. Return the pointer to the child.
    // DO NOTHING and Return NULL if p is NULL.
    Nodeptr insert(Nodeptr p, T const & value)
    {
        if (!p) return Nodeptr();
        // Pointer to the child.
        Nodeptr const child = p.m_insert(value);
        // Adjust sizes of ancestors.
        p.m_ptr->incsize(child.m_ptr->size = 1);

        return child;
    }
    // Check data structure integrity.
    // DO NOTHING and Return true if p is NULL.
    bool check(Nodeptr p) const
    {
        if (!p) return true;
        size_type n = 0;
        FOR (Nodeptr child, p.m_ptr->children)
        {
            if (child.parent() != p) return false;
            n += child.size();
        }
        if (p.size() != n + 1) return false;
        FOR (Nodeptr child, p.m_ptr->children)
            if (!check(child)) return false;
        return true;
    }
    bool check() const { return check(data()); }
};

// Return 0 -> 1 -> ... -> n.
template<class T> Tree<T> chain1(T n)
{
    Tree<T> t(0);
    Tree<T>::Nodeptr p(t.data());
    for (T i = 1; i <= n; ++i) p = t.insert(p, i);
    return t;
}

#endif // TREE_H_INCLUDED
