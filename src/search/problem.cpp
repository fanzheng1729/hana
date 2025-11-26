#include <cctype>   // for std::tolower
#include <fstream>
#include "../proof/analyze.h"
#include "../database.h"
#include "goaldata.h"
#include "../io.h"
#include "problem.h"

// UCB threshold for generating a new batch of moves
// Change this to turn on staged move generation.
Value Problem::UCBnewstage(Nodeptr p) const
{
    if (!(staged & STAGED))
        return MCTS<Game>::UCBnewstage(p);
    // Staged
    if (!isourturn(p))
        return std::numeric_limits<Value>::max();
    // Our turn
    Game const & game = p->game();
    stage_t const stage = p->stage();
    Proofsize size = game.env().hypslen + game.goal().size() + stage;
    size_type const self = static_cast<size_type>(1) << (stage*2);
    return score(size) + UCBbonus(true, p.size(), self);
}

// Evaluate the leaf. Return {value, sure?}.
// p should != NULL.
Eval Problem::evalleaf(Nodeptr p) const
{
    Game const & game = p->game();
    if (game.attempt.type == Move::THM)
    {
        bool loops(Nodeptr p);
        if (loops(p))
            return EvalLOSS;
    }

    if (!isourturn(p))
        return evaltheirleaf(p);
    // Our leaf
    if (!game.pGoal || !game.pEnv())
        return EvalLOSS;
    if (p.parent() && game.proven())
        return EvalWIN;
    if (!p.parent() && proven(game.pGoal, assertion))
        return EvalWIN;

    return game.env().evalourleaf(game);
}

Eval Problem::evaltheirleaf(Nodeptr p) const
{
    Value value = WDL::WIN;
    if (const_cast<Problem *>(this)->expand<&Game::moves>(p))
    {
        evalnewleaves(p);
        value = minimax(p);
    }

    if (value == WDL::WIN)
    {
        if (!p->game().writeproof())
            return EvalLOSS;
        const_cast<Problem *>(this)->closenodes(p);
        return EvalWIN;
    }
    // value is between WDL::LOSS and WDL::WIN.
    if (p->game().nDefer == 0)
        FOR (Nodeptr child, *p.children())
            addNodeptr(child);
    return Eval(value, false);
}

void Problem::closemorenodes(Nodeptr p)
{
    Goaldatas const & goaldatas = p->game().goaldata().pBigGoal->second;
    Environs::const_iterator to = p->game().env().enviter;
    FOR (Goaldatas::const_reference goaldata, goaldatas)
    {
        Environs::const_iterator from = goaldata.first->enviter;
        if (!environs.reachable(from, to)) continue;
        FOR (Nodeptr const other, goaldata.second.nodeptrs)
            if (!other->won())
            {
                std::cout << goaldata.first->label() << "\n->\n" << p->game().env().label();
                other->game().proof() = p->game().proof();
                Nodeptr const parent = other.parent();
                if (parent && !parent->won())
                    backprop(parent);
            }
    }
}

static void printDAGcycle(strview env1, strview env2)
{
    std::cerr << "cycle formed by\n" << env1 << "\n->\n" << env2 << std::endl;
}

// Add a sub-context for the game. Return pointer to the new context.
// Return NULL if not okay.
Environ * Problem::addsubEnv(Environ const * pEnv, Bvector const & hypstotrim)
{
    if (hypstotrim.empty())
        return NULL;
    // Name of new context
    std::string const & label(pEnv->assertion.hypslabel(hypstotrim));
    // Try add the context.
    std::pair<Environs::iterator, bool> const result
    = environs.insert(std::pair<strview, Environ *>(label, NULL));
    // Iterator to the new context
    Environs::iterator const newenviter = result.first;
    if (!environs.linked(pEnv->enviter, newenviter) &&
        !environs.link(pEnv->enviter, newenviter))
        printDAGcycle(pEnv->enviter->first, newenviter->first);
    // If it already exists, set the game's context pointer.
    if (!result.second)
        return newenviter->second;
    // If it does not exist, add the simplified assertion.
    Assertion & newass = assertions[newenviter->first];
    if (unexpected(!newass.expression.empty(), "Duplicate label", label))
        return NULL;
    Assertion const & ass = pEnv->makeass(hypstotrim);
    if (ass.expression.empty())
        return NULL;
    // Add the new context.
    Environ * const pnewEnv = pEnv->makeenv(newass = ass);
    if (pnewEnv)
    {
        (newenviter->second = pnewEnv)->pProb = this;
        pnewEnv->enviter = newenviter;
    }
    return pnewEnv;
}

// Add a goal. Return its pointer.
Goalptr Problem::addGoal(Goalview const & goal, Environ * pEnv, Goalstatus s)
{
    BigGoalptr pBigGoal = &*goals.insert(std::make_pair(goal, Goaldatas())).first;
    Goalptr pGoal = &*pBigGoal->second.insert(std::make_pair(pEnv, s)).first;
    pGoal->second.pBigGoal = pBigGoal;
    return pGoal;
}

// Return the only open child of p.
// Return NULL if p has 0 or at least 2 open children.
static Nodeptr onlyopenchild(Nodeptr p)
{
    if (Problem::isourturn(p)) return Nodeptr();

    Nodeptr result;
    bool hasopenchild = false;
    FOR (Nodeptr child, *p.children())
    {
        if (child->won()) continue;
        // Open child found.
        if (hasopenchild) return Nodeptr();
        // 1 open child
        result = child;
        hasopenchild = true;
    }
    return result;
}

// Move to the only open child of a node. Return true if it exists.
static bool gotoonlyopenchild(Nodeptr & p)
{
    if (Nodeptr child = onlyopenchild(p))
        return p = child;
    return false;
}

static const char strproven[] = "V";

// Format: ax-mp[!]
static void printrefname(Nodeptr p)
{
    std::cout << p->game().attempt.label();
    if (onlyopenchild(p)) std::cout << '!';
}

// Format: ax-mp[!] or DEFER(n)
static void printattempt(Nodeptr p)
{
    switch (p->game().attempt.type)
    {
    case Move::DEFER :
        std::cout << "DEFER(" << p->game().nDefer << ')';
        break;
    case Move::THM :
        printrefname(p);
        break;
    default :
        std::cout << "NONE";
    }
}
// Format: score*size
static void printeval(Nodeptr p)
{
    std::cout << ' ' << Problem::value(p) << '*' << p.size();
}
// Format: score*size=UCB
static void printeval(Nodeptr p, Problem const & tree)
{
    printeval(p); std::cout << '=' << tree.UCB(p) << '\t';
}

// Format: ax-mp score*size=UCB ...
static void printourchildren(Nodeptr p, Problem const & tree)
{
    FOR (Nodeptr child, *p.children())
        printattempt(child), printeval(child, tree);
    std::cout << std::endl;
}

// Format: (n) if n > 0
static void printstage(stage_t stage)
{
    if (stage)
        std::cout << '(' << stage << ") ";
}

// Format: [(n)] maj score*size   [V]|- ...
static void printournode(Nodeptr p, stage_t stage)
{
    Move const & lastmove = p.parent()->game().attempt;
    printstage(stage);
    std::cout << lastmove.hyplabel(lastmove.matchhyp(p->game().goal()));
    printeval(p);
    std::cout << '\t';
    std::cout << &strproven[!p->game().proven()];
    std::cout << p->game().goal().expression();
}

// Format: DEFER(n) score*size
static void printdeferline(Nodeptr p)
{
    printattempt(p);
    if (!p.children()->empty())
        printeval((*p.children())[0]);
    std::cout << std::endl;
}
// Format: min score*size maj score*size
static void printhypsline(Nodeptr p)
{
    Hypsize i = 0;
    FOR (Nodeptr child, *p.children())
    {
        while (p->game().attempt.hypfloats(i)) ++i;
        std::cout << p->game().attempt.hypiter(i++)->first;
        printeval(child);
        std::cout << ' ';
    }
    std::cout << std::endl;
}

// Format: ax-mp[!] score*size score*size
static void printtheirnode(Nodeptr p)
{
    printrefname(p);
    FOR (Nodeptr child, *p.children())
        printeval(child);
    std::cout << std::endl;
}

// Format:
// Goal score [V]|- ...
// [Hyps hyo1 hyp2]
static void printgoal(Nodeptr p)
{
    std::cout << "Goal " << Problem::value(p) << ' ';
    std::cout << &strproven[!p->game().proven()];
    std::cout << p->game().goal().expression();

    Assertion const & ass = p->game().env().assertion;
    if (ass.esshypcount() == 0) return;

    std::cout << "Hyps ";
    FOR (Hypiter iter, ass.hypiters)
        if (!iter->second.floats)
            std::cout << iter->first << ' ';
    std::cout << std::endl;
}

// Format:
/**
 | Goal score |- ...
 | [Hyps hyp1 hyp2]
 | ax-mp score*size=UCB ...
 | DEFER(n) score*size OR
 | min score*size maj score*size
**/
// n.   (n) ax-mp[!] score*size score*size
//      maj score*size  |- ...
void Problem::printmainline(Nodeptr p, size_type detail) const
{
    printgoal(p);
    // Print children.
    static void (*const printfn[])(Nodeptr) = {&printhypsline, &printdeferline};
    if (detail)
        isourturn(p) ? printourchildren(p, *this) :
            (*printfn[p->game().attempt.type == Move::DEFER])(p);
// std::cout << "Children printed" << std::endl;
    size_type level = 0, nDefer = 0;
    while (p = pickchild(p))
    {
        if (!isourturn(p))
        {
            switch (p->game().attempt.type)
            {
            case Move::DEFER:
                ++nDefer;
                continue;
            case Move::THM:
                std::cout << ++level << ".\t";
                printstage(nDefer);
                nDefer = 0;
                printtheirnode(p);
                continue;
            case Move::NONE:
                std::cerr << "Empty move" << std::endl;
                return;
            }
        }
        else if (p->game().nDefer == 0)
        {
            std::cout << '\t';
            printournode(p, p->stage() * (staged & STAGED));
            if (detail > level)
                printourchildren(p, *this);
        }
    }
}

void Problem::checkmainline(Nodeptr p) const
{
    for ( ; p; p = pickchild(p))
    {
        if (isourturn(p) && p->game().nDefer == 0
            && p->game().proven() && !p->won())
        {
            for (Nodeptr q = root(); q && q != p; q = pickchild(q))
                std::cout << *q;
            std::cout << *p;
            navigate();
        }
    }
}

// Format: n nodes, x V, y ?, z X in m contexts
void Problem::printstats() const
{
    std::cout << playcount() << " plays, " << size() << " nodes, ";
    std::cout << countproof() << '/';
    static const char * const s[] = {" V, ", " ?, ", " X in "};
    for (int i = GOALTRUE; i >= GOALFALSE; --i)
        std::cout << countgoal(i) << s[GOALTRUE - i];
    std::cout << countenvs() << " contexts " << std::endl;
    unexpected(countgoal(GOALNEW) > 0, "unevaluated", "goal");
}

// Move up to the parent. Return true if successful.
static bool moveup(Nodeptr & p)
{
    if (!p.parent()) return false;

    p = p.parent();
    if (onlyopenchild(p) && p.parent() &&
        askyn("Go to grandparent y/n?"))
        p = p.parent();

    return true;
}

// Input the index and move to the child. Return true if successful.
static bool gototheirchild(Nodeptr & p)
{
    std::size_t i;
    std::cin >> i;

    Problem::Children const & children(*p.children());
    return i >= children.size() ? Nodeptr() : (p = children[i]);
}

// Move to the child with given assertion and index. Return true if successful.
static bool findourchild(Nodeptr & p, strview ass, std::size_t index)
{
    FOR (Nodeptr child, *p.children())
    {
        Move const & move = child->game().attempt;
        if (move.type == Move::THM && move.label() == ass && index-- == 0)
            return p = child;
    }
    return false;
}

// Input the assertion and the index and move to the child. Return true if successful.
static bool gotoourchild(Nodeptr & p)
{
    if (p.children()->empty())
        return false;

    std::string token;
    std::cin >> token;
    // Find the child named by token.
    if (token[0] == '*')
        p = p.children()->back();
    else
    {
        std::size_t i;
        std::cin >> i;
        if (!findourchild(p, token, i))
            return false;
    }

    Nodeptr grandchild = p;
    if (gotoonlyopenchild(grandchild) &&
        askyn("Go to only open child y/n?"))
        p = grandchild;
    return true;
}

void Problem::navigate(bool detailed) const
{
    if (empty()) return;

    std::cout <<
    "On our turn,\n"
    "c [name] [n] changes to the n-th node using the assertion [name]\n"
    "c * changes to the last node\n"
    "On their turn, c [n] changes to the n-th hypothesis\n"
    "u[p] goes up a level\nh[ome] or t[op] goes to the top level\n"
    "b[ye] or e[xit] or q[uit] to leave navigation" << std::endl;

    std::string token;
    Nodeptr p = root();
    while (true)
    {
// std::cout << isourturn(node) << ' ' << &node.value() << std::endl;
        std::cin.clear();
        std::cout << "> ";

        std::cin >> token;
        unsigned char const c = token[0];
        switch (std::tolower(c))
        {
        case 'b': case 'e': case 'q':
            return;
        case 'u':
            moveup(p);
            break;
        case 'h': case 't':
            p = root();
            break;
        case 'c':
            isourturn(p) ? gotoourchild(p) : gototheirchild(p);
            break;
        default:
            std::cout << "Invalid command. Try again\n";
            continue;
        }
        printmainline(p, detailed);
    }

    static std::string const hline(50, '-');
    std::cout << hline << std::endl;
}

void Problem::writeproof(const char * const filename) const
{
    if (proof().empty() || !root()->game().pEnv() || !filename)
        return;

    Printer printer(&root()->game().env().database.typecodes());
    verify(proof(), printer);

    std::ofstream out(filename);
    out << printer.str(indentation(ast(proof())));
}
