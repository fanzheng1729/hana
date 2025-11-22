#ifndef STATNODE_H_INCLUDED
#define STATNODE_H_INCLUDED

#include <iostream>
#include "stageval.h"

// Game state = {game, bool = is our turn?}
template<class G> class State : G
{
    bool m_ourturn;
public:
    using typename G::Move;
    template<class T>
    State(T const & game, bool isourturn = true) :
        G(game), m_ourturn(isourturn) {}
    G const & game() const { return *this; }
    bool isourturn() const { return m_ourturn; }
    using G::moves;
    bool legal(Move const & move) const
    { return G::legal(move, m_ourturn); }
    State play(Move const & move) const
    {
        State result(G::play(move, m_ourturn));
        result.m_ourturn = m_ourturn ^ 1;
        return result;
    }
    friend std::ostream & operator<<(std::ostream & out, State const & state)
    {
        static char const s[2][4] = {"Thr", "Our"};
        out << s[state.isourturn()] << " turn\n" << state.game();
        return out;
    }
};

// Monto-Carlo search tree
template<class G> struct MCTS;

// Node base = {game state, evaluation}
template<class G> class NodeBase : public State<G>, private Eval
{
    void seteval(Eval eval) { *static_cast<Eval *>(this) = eval; }
    friend MCTS<G>;
public:
    template<class T>
    NodeBase(T const & game) : State<G>(game) {}
    Eval eval() const { return *this; }
    bool won() const { return *this == EvalWIN; }
    bool drawn() const { return *this == EvalDRAW; }
    bool lost() const { return *this == EvalLOSS; }
    friend std::ostream & operator<<(std::ostream & out, NodeBase const & node)
    {
        out << "Eval: " << node.value << &"*\t"[!node.sure];
        out << static_cast<State<G> const &>(node) << std::endl;
        return out;
    }
};

// Return true if a game supports staged move generation
template<class G> class Staged
{
    typedef char N, Y[2];
    static N & f(typename G::Moves (G::*)(bool) const){}
    static Y & f(typename G::Moves (G::*)(bool, stage_t) const){}
public:
    static const bool value = sizeof(f(&G::moves)) - 1;
};

// Stage count, if Game supports staged move generation
template<class G, bool = Staged<G>::value> class Stage;
template<class G> class Stage<G, true>
{
    // Stage of move generation
    stage_t m_stage;
    // # children evaluated
    std::size_t m_index;
    friend MCTS<G>;
public:
    Stage() : m_stage(0), m_index(0) {}
    stage_t stage() const { return m_stage; }
    std::size_t index() const { return m_index; }
    void setindex(std::size_t index) { m_index = index; }
};
template<class G> struct Stage<G, false>
{
    stage_t stage() const { return 0; }
    std::size_t index() const { return 0; }
    void setindex(std::size_t) {}
};

// MCTS node = {node base, Stage}
template<class G> struct MCTSNode : NodeBase<G>, Stage<G>
{
    template<class T> MCTSNode(T const & game) : NodeBase<G>(game) {}
};

#endif // STATNODE_H_INCLUDED
