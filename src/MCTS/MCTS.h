#ifndef MCTS_H_INCLUDED
#define MCTS_H_INCLUDED

#include <algorithm>    // for std::copy and std::..._element
#include <cmath>        // for std::sqrt
#include <iostream>
#include "statnode.h"
#include "tree.h"
#include "../util/arith.h"
#include "../util/for.h"

// Monte-Carlo search tree
template<class G>
struct MCTS : private Tree<MCTSNode<G> >
{
    typedef typename G::Moves Moves;
    typedef Tree<MCTSNode<G> > MCTSTree;
    using typename MCTSTree::size_type;
    using typename MCTSTree::Nodeptr;
    using typename MCTSTree::Children;
private:
    static int const digits = std::numeric_limits<size_type>::digits;
    // Exploration constant
    Value m_exploration[2];
    // Caches for square roots
    Value sqrt[2][digits];
    // Value sqrt2[digits];
    // Bonus for UCB
    // Value m_UCBbonus[2][digits][digits];
    // Initialize the cache.
    void initcache()
    {
        for (int b = 0; b < 2; ++b)
            for (int i = 0; i < digits; ++i)
                sqrt[b][i] = m_exploration[b] * std::sqrt(i);

//        for (int i = 0; i < digits; ++i)
//            sqrt2[i] = std::sqrt(1 << i);
//
//        for (int b = 0; b < 2; ++b)
//            for (int i = 0; i < digits; ++i)
//                for (int j = 0; j < digits; ++j)
//                    m_UCBbonus[b][i][j] = sqrt[b][i]/sqrt2[j];

    }
    // # playouts
    size_type m_playcount;
public:
    // Construct a tree with 1 node.
    template<class T>
    MCTS(T const & game, Value const exploration[2]) :
        MCTSTree(game), m_playcount(0)
    {
        std::copy(exploration, exploration + 2, m_exploration);
        initcache();
    }
    using MCTSTree::clear;
    using MCTSTree::data;
    using MCTSTree::root;
    using MCTSTree::empty;
    using MCTSTree::size;
    Value const * exploration() const { return m_exploration; }
    static bool issure(Nodeptr p) { return p->eval().sure; }
    bool issure() const { return issure(root()); }
    static Value value(Nodeptr p) { return p->eval().value; }
    Value value() const { return value(root()); }
protected:
    static void seteval(Nodeptr p, Eval eval) { if (p) p->seteval(eval); }
    static void setwin (Nodeptr p) { if (p) p->seteval(EvalWIN); }
    static void setdraw(Nodeptr p) { if (p) p->seteval(EvalDRAW); }
    static void setloss(Nodeptr p) { if (p) p->seteval(EvalLOSS); }
public:
    static bool isourturn(Nodeptr p) { return p->isourturn();}
    // Return true if value of x < value of y.
    static bool compvalue(Nodeptr x, Nodeptr y) { return value(x) < value(y); }
    // Return the UCB bonus term
    Value UCBbonus(bool ourturn, size_type parent, size_type self) const
    { return sqrt[ourturn][util::log2(parent)] / std::sqrt(self); }
    // Compute the upper confidence bound.
    Value UCB(Nodeptr p) const
    {
        Value const v = value(p);
        if (::issure(v)) return v;
        return v + UCBbonus(!isourturn(p), p.parent().size(), p.size());
    }
    // Compare 2 children, by UCB and turn.
    // < 0 if x < y, = 0 if x == y, > 0 if x > y.
    int compchild(Nodeptr x, Nodeptr y) const
    {
        Value const dif = UCB(x) - UCB(y);
        int const raw = dif > 0 ? 1 : dif < 0 ? -1: 0;
        return !isourturn(x) ? raw : -raw;
    }
    // UCB threshold for generating a new batch of moves
    // Change this to turn on staged move generation.
    virtual Value UCBnewstage(Nodeptr p) const
    {
        static Value const inf = std::numeric_limits<Value>::max();
        return isourturn(p) ? -inf : inf;
    }
    // Return the unsure child with largest UCB.
    // Return NULL if there is no such a child.
    Nodeptr pickchild(Nodeptr p) const
    {
        Children const * const children = p.children();
        if (!children) return Nodeptr();
        // Find the first unsure child.
        typedef typename Children::const_iterator Iter;
        Iter child = children->begin();
        for ( ; child != children->end(); ++child)
            if (!issure(*child)) break;
        // If all children are sure, return NULL.
        if (child == children->end())
            return Nodeptr();
        // Find the unsure child with largest UCB.
        for (Iter iter(child); iter != children->end(); ++iter)
        {
            if (issure(*iter)) continue;
            if (compchild(*child, *iter) < 0) child = iter;
        }
        // Determine whether to generate a new batch of moves.
        if (isourturn(p) ? UCB(*child) < UCBnewstage(p) :
            UCB(*child) > UCBnewstage(p))
            return Nodeptr();

        return *child;
    }
    // Return the leaf with largest UCB.
    // Return NULL if p is NULL.
    Nodeptr pickleaf(Nodeptr p) const
    {
        Nodeptr result;
        while (p) result = p, p = pickchild(p);
        return result;
    }
    // Expand the node pointed. Return # new children.
    // p should != NULL.
    template<Moves (G::*)(bool) const>
    size_type expand(Nodeptr p)
    {
        return addchildren(p, p->moves(isourturn(p)));
    }
    template<Moves (G::*)(bool, stage_t) const>
    size_type expand(Nodeptr p)
    {
        stage_t & stage = p->m_stage;
        return addchildren(p, p->moves(isourturn(p), stage++));
    }
    // Call back when children of p moved.
    virtual void expandcallback(Nodeptr p) {}
    // Evaluate the leaf. Return {value, sure?}.
    // p should != NULL.
    virtual Eval evalleaf(Nodeptr p) const = 0;
    // Returns the minimax value of a parent.
    // p should != NULL.
    static Value minimax(Nodeptr p)
    {
        Children const & ch = *p.children();
        return ch.empty() ? WDL::DRAW :
            value(isourturn(p) ?
                  *std::max_element(ch.begin(), ch.end(), compvalue) :
                  *std::min_element(ch.begin(), ch.end(), compvalue));
    }
    // Evaluate the parent. Return {value, sure?}.
    // p should != NULL.
    virtual Eval evalparent(Nodeptr p) const { return minimax(p); }
    // Evaluate all the new leaves.
    // p should != NULL.
    void evalnewleaves(Nodeptr p) const
    {
        for (size_type i = p->index(); i < p.children()->size(); ++i)
        {
            Nodeptr const child = (*p.children())[i];
            // Evaluate child.
            if (!child.children()->empty())
                continue; // child not a leaf
            Eval const eval = evalleaf(child);
            child->seteval(eval);
            if (isourturn(p) && eval == EvalWIN)
                break;
            if (!isourturn(p) && eval == EvalLOSS)
                break;
        }
        p->setindex(p.children()->size());
    }
    // Evaluate the node. Return {value, sure?}.
    // p should != NULL.
    Eval evaluate(Nodeptr p) const
    { return p.children()->empty() ? evalleaf(p) : evalparent(p); }
    // Call back for back propagation.
    virtual void backpropcallback(Nodeptr p) {}
    // Back propagate from the node pointed.
    // DO NOTHING if p is NULL.
    void backprop(Nodeptr p)
    {
// std::cout << "Back prop called on " << p;
        for ( ; p; p = p.parent())
        {
// std::cout << "Back prop to " << *p;
            p->seteval(evaluate(p));
            backpropcallback(p);
        }
    }
    size_type playcount() const { return m_playcount; }
    virtual void checkmainline(Nodeptr p) const {}
    // Play out once. Return the value at the root.
    Value playonce()
    {
// std::cout << playcount() << '\t' << size() << std::endl;
// std::cout << *root();
// checkmainline(root());
        Nodeptr p = pickleaf(root());
// std::cout << "Expanding " << *p;
        if (size_type const n = expand<&G::moves>(p))
// std::cout << "Expanded " << n << " new moves at " << *p,
            evalnewleaves(p);
// std::cout << "Back propagating from " << *p;
        backprop(p);
        ++m_playcount;
        return value();
    }
    // Play out until the value is sure or size limit is reached.
    void play(size_type maxsize)
    {
        if (empty() || issure()) return;
        root()->seteval(evalleaf(root()));
        for ( ; !issure(); playonce())
        {
            if (size() > maxsize) break;
// showeval();
// std::cout << "new leaf = " << *pickleaf(root());
// std::cin.get();
        }
    }
    void showeval() const
    {
        std::cout << "value = " << value() << " size = " << size();
        std::cout << &" not"[issure()] << std::endl;
    }
    virtual ~MCTS() {}
private:
    // For all functions below, p should != NULL.
    // Add children. Return # new children.
    size_type addchildren(Nodeptr p, Moves const & moves)
    {
// std::cout << "Adding " << moves.size() << " moves to " << *p;
        size_type const oldsize = p.children()->size();
        if (p.reserve(oldsize + moves.size()))
            expandcallback(p);

        FOR (typename Moves::const_reference move, moves)
        {
            if (!p->legal(move)) continue;
            // Add child.
            Nodeptr child(this->insert(p, p->play(move)));
        }
// if (p->stage() >= 5)
// std::cout << p.children()->size() - oldsize << " moves added to " << *p;
        return p.children()->size() - oldsize;
    }
};

#endif // MCTS_H_INCLUDED
