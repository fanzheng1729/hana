#include "../../database.h"
#include "../problem.h"

// Return true if a new batch of moves is needed.
// Override this to turn on staged move generation.
bool Problem::needwidening(pNode p, pNode child) const
{
    return staged && isourturn(p) &&
            // p.size() > p.nchild() * p.nchild();
            UCB(child) < UCBwidening(p);
}
// UCB threshold for generating a new batch of moves
// Override this to turn on staged move generation.
Value Problem::UCBwidening(pNode p) const
{
    Treesize const self = static_cast<Treesize>(1) << (p->stage()*2);
    return score(p->game().env().weight(p->game()) + p->stage())
            + UCBbonus(true, p.size(), self);
}

// Do singular extension. Return the value.
// p should != nullptr.
Value Problem::singularext(pNode p)
{
    if (isourturn(p)) return value(p);
    // Their turn
    Value value = WDL::WIN;
    if (expand<&Game::moves>(p))
    {
        evalnewleaves(p);
        value = minimax(p);
    }
    return value;
}

// Return true if game's rank < Problem's rank.
bool Problem::ranksimplerthanProb(Game const & game) const
{
    return game.env().rankssimplerthanProb &&
        database().syntaxDAG().simplerthan
        (database().syntaxDAG().RPNranks(game.goal().rpn), maxranks);
}

// Evaluate the leaf. Return {value, sure?}.
// p should != nullptr.
Eval Problem::evalleaf(pNode p) const
{
    Game const & game = p->game();
    if (game.attempt.type == Move::THM)
    {
        bool loops(pNode p);
        if (loops(p))
            return EvalLOSS;
    }

    // if (isourturn(p) && p.parent() && p.parent()->game().attempt.isconj())
    //     navigate(p);
    return !isourturn(p) ? evaltheirleaf(p) :
    // Our leaf
        game.proven() ? EvalWIN :
        ranksimplerthanProb(game) ? ALMOSTWIN :
        game.env().evalourleaf(game);
}

Eval Problem::evaltheirleaf(pNode p) const
{
    Value value = const_cast<Problem *>(this)->singularext(p);
    if (value == WDL::WIN)
    {
        if (!p->game().writeproof())
            return EvalLOSS;
        const_cast<Problem *>(this)->closenodes(p);
        return EvalWIN;
    }
    // value is between WDL::LOSS and WDL::WIN.
    if (!p->game().nDefer)
        FOR (pNode child, *p.children())
            addpNode(child);
    return Eval(value, false);
}

// Evaluate the parent. Return {value, sure?}.
// p should != nullptr.
Eval Problem::evalparent(pNode p) const
{
    Value   const v = minimax(p);
    bool    const stuck = staged && isourturn(p) && v == WDL::LOSS;
    return  stuck ? p->eval() : v;
}
