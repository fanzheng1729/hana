#include <cctype>   // for std::tolower
#include <fstream>
#include "../proof/analyze.h"
#include "../database.h"
#include "goaldata.h"
#include "../io.h"
#include "problem.h"

static const char strproven[] = "V";

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
        if (int i = loops(p))
            return EvalLOSS;
    }

    if (!isourturn(p))
        return evaltheirleaf(p);
    // Our leaf
    if (p.parent() && game.proven())
        return EvalWIN;
    if (!p.parent() && proven(game.pgoal, assertion))
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
    if (p->game().ndefer == 0)
        FOR (Nodeptr child, *p.children())
            addnodeptr(child);
    return Eval(value, false);
}

// Add a environment for the game.
Environ * Problem::addenv(Environ const * penv, Bvector const & hypstotrim)
{
    // Name of environment
    std::string const & label(penv->assertion.hypslabel(hypstotrim));
    // Try add the environment.
    std::pair<Environs::iterator, bool> const result
    = environs.insert(std::pair<strview, Environ *>(label, NULL));
    // If it already exists, set the game's environment pointer.
    if (!result.second)
        return result.first->second;
    // Iterator to the environment
    Environs::iterator const enviter = result.first;
    // Add the simplified assertion.
    Assertion & newass = assertions[enviter->first];
    if (unexpected(!newass.expression.empty(), "Duplicate label", label))
        return NULL;
    Assertion const & ass = penv->makeass(hypstotrim);
    if (ass.expression.empty())
        return NULL;
    // Add the new environment.
    if (Environ * const pnewenv = penv->makeenv(newass = ass))
        return (enviter->second = pnewenv)->pProb = this, pnewenv;
    return NULL;
}

// Add a goal. Return its pointer.
Goal2ptr Problem::addgoal(Proofsteps const & RPN, strview typecode, Goalstatus s)
{
    return NULL;
}

typedef Problem::Nodeptr Nodeptr;

// If goal appears as the goal of a node or its ancestors,
// return the pointer of the ancestor.
// This check is necessary to prevent self-assignment in writeproof().
static Nodeptr loops(Goalptr pgoal, Nodeptr pnode)
{
    while (true)
    {
        Game const & game = pnode->game();
        if (game.ndefer == 0 && pgoal == game.pgoal)
            return pnode;
        if (Nodeptr const parent = pnode.parent())
            pnode = parent.parent();
        else break;
    }
    return Nodeptr();
}

// Return true if ptr duplicates upstream goals.
bool loops(Nodeptr p)
{
    Move const & move = p->game().attempt;
    // All the goals necessary to prove p
    Goalptrs allgoals;
    // Check if any of the hypotheses appears in a parent node.
    for (Hypsize i = 0; i < move.hypcount(); ++i)
    {
        if (move.hypfloats(i))
            continue;
        Goalptr const pgoal = move.hypvec[i];
        if (pgoal->second.proven())
            continue;
        if (loops(pgoal, p.parent()))
            return true;
        allgoals.insert(pgoal);
    }
    // Check if these hypotheses combined prove a parent node.
    while (Goalptr const pnewgoal = allgoals.saturate())
        FOR (Nodeptr const pnewnode, pnewgoal->second.nodeptrs)
            if (pnewnode.isancestorof(p))
                return true;
    return false;
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
        if (Problem::value(child) == WDL::WIN) continue;
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
        std::cout << "DEFER(" << p->game().ndefer << ')';
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
    Game const & game = p->game();
    Goal const & goal = game.goal();
    printstage(stage);
    std::cout << lastmove.hyplabel(lastmove.matchhyp(goal));
    printeval(p);
    std::cout << '\t';
    std::cout << &strproven[!game.proven()];
    std::cout << goal.expression();
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
    Move const & move = p->game().attempt;
    Hypsize i = 0;
    FOR (Nodeptr child, *p.children())
    {
        while (move.hypfloats(i)) ++i;
        std::cout << move.hypiter(i++)->first;
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
    Game const & game = p->game();
    std::cout << &strproven[!game.proven()];
    std::cout << game.goal().expression();

    Assertion const & ass = game.env().assertion;
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
    size_type level = 0, ndefer = 0;
    while (p = pickchild(p))
    {
        if (!isourturn(p))
        {
            Move const & move = p->game().attempt;
            switch (move.type)
            {
            case Move::DEFER:
                ++ndefer;
                continue;
            case Move::THM:
                std::cout << ++level << ".\t";
                printstage(ndefer);
                ndefer = 0;
                printtheirnode(p);
                continue;
            default:
                std::cerr << "Bad move" << move << std::endl;
                return;
            }
        }
        else if (p->game().ndefer == 0)
        {
            std::cout << '\t';
            printournode(p, (p->stage()) * (staged & STAGED));
            if (detail > level)
                printourchildren(p, *this);
        }
    }
}

void Problem::checkmainline(Nodeptr p) const
{
    for ( ; p; p = pickchild(p))
    {
        if (isourturn(p) && p->game().ndefer == 0
            && p->game().proven() && value(p) != WDL::WIN)
        {
            for (Nodeptr q = root(); q && q != p; q = pickchild(q))
                std::cout << *q;
            std::cout << *p;
            navigate();
        }
    }
}

// Format: n nodes, x V, y ?, z X in m contests
void Problem::printstats() const
{
    static const char * const s[] = {" V, ", " ?, ", " X in "};
    std::cout << playcount() << " plays, " << size() << " nodes, ";
    for (int i = WDL::WIN; i >= WDL::LOSS; --i)
        std::cout << countgoal(i) << s[WDL::WIN - i];
    std::cout << countenvs() << " environs " << std::endl;
}

// One environment a line
void Problem::printenvs() const
{
    FOR (Environs::const_reference env, environs)
        std::cout << env.first << std::endl;
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
    if (i >= children.size()) return false;
    return p = children[i];
}

// Move to the child with given assertion and index. Return true if successful.
static bool findourchild(Nodeptr & p, strview ass, std::size_t index)
{
    FOR (Nodeptr child, *p.children())
    {
        Move const & move(child->game().attempt);
        if (move.type == Move::THM && move.label() == ass && index-- == 0)
            return p = child;
    }
    return false;
}

// Input the assertion and the index and move to the child. Return true if successful.
static bool gotoourchild(Nodeptr & p)
{
    if (p.children()->empty()) return false;

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
    if (proof().empty())
        return;

    Printer printer(&root()->game().env().database.typecodes());
    verify(proof(), printer);

    std::ofstream out(filename);
    out << printer.str(indentation(ast(proof())));
}
