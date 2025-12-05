#include <cctype>   // for std::tolower
#include <fstream>
#include "../../io.h"
#include "../problem.h"
#include "../../proof/analyze.h"
#include "../../proof/printer.h"
#include "../../proof/verify.h"

// Return the only open child of p.
// Return NULL if p has 0 or at least 2 open children.
static pNode onlyopenchild(pNode p)
{
    if (Problem::isourturn(p)) return pNode();

    pNode result;
    bool hasopenchild = false;
    FOR (pNode child, *p.children())
    {
        if (child->won()) continue;
        // Open child found.
        if (hasopenchild) return pNode();
        // 1 open child
        result = child;
        hasopenchild = true;
    }
    return result;
}

// Move to the only open child of a node. Return true if it exists.
static bool gotoonlyopenchild(pNode & p)
{
    if (pNode child = onlyopenchild(p))
        return p = child;
    return false;
}

static const char strproven[] = "V";

// Format: ax-mp[!]
static void printrefname(pNode p)
{
    std::cout << p->game().attempt.thmlabel();
    if (onlyopenchild(p)) std::cout << '!';
}

// Format: ax-mp[!] or DEFER(n)
static void printattempt(pNode p)
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
static void printeval(pNode p)
{
    std::cout << ' ' << Problem::value(p) << '*' << p.size();
}
// Format: score*size=UCB
static void printeval(pNode p, Problem const & tree)
{
    printeval(p); std::cout << '=' << tree.UCB(p) << '\t';
}

// Format: ax-mp score*size=UCB ...
static void printourchildren(pNode p, Problem const & tree)
{
    FOR (pNode child, *p.children())
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
static void printournode(pNode p, stage_t stage)
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
static void printdeferline(pNode p)
{
    printattempt(p);
    if (p.haschild())
        printeval((*p.children())[0]);
    std::cout << std::endl;
}
// Format: min score*size maj score*size
static void printhypsline(pNode p)
{
    Hypsize i = 0;
    FOR (pNode child, *p.children())
    {
        while (p->game().attempt.hypfloats(i)) ++i;
        std::cout << p->game().attempt.hyplabel(i++);
        printeval(child);
        std::cout << ' ';
    }
    std::cout << std::endl;
}

// Format: ax-mp[!] score*size score*size
static void printtheirnode(pNode p)
{
    printrefname(p);
    FOR (pNode child, *p.children())
        printeval(child);
    std::cout << std::endl;
}

// Format:
// Goal score [V]|- ...
// [Hyps hyo1 hyp2]
static void printgoal(pNode p)
{
    std::cout << "Goal " << Problem::value(p) << ' ';
    std::cout << &strproven[!p->game().proven()];
    std::cout << p->game().goal().expression();

    Assertion const & ass = p->game().env().assertion;
    if (ass.esshypcount() == 0) return;

    std::cout << "Hyps ";
    for (Hypsize i = 0; i < ass.hypcount(); ++i)
        if (!ass.hypfloats(i))
            std::cout << ass.hyplabel(i) << ' ';
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
void Problem::printmainline(pNode p, size_type detail) const
{
    printgoal(p);
    // Print children.
    static void (*const printfn[])(pNode) = {&printhypsline, &printdeferline};
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

void Problem::checkmainline(pNode p) const
{
    for ( ; p; p = pickchild(p))
    {
        if (isourturn(p) && p->game().nDefer == 0
            && p->game().proven() && !p->won())
        {
            for (pNode q = root(); q && q != p; q = pickchild(q))
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

// Format: [*]name  maxrank1 maxrank2 ...
void Problem::printenvs() const
{
    FOR (Environs::const_reference env, environs)
    {
        SyntaxDAG::Ranks const & envranks = env.second->maxranks;
        char const c = " *"[env.second->rankssimplerthanProb()];
        std::cout << c << env.first << '\t';
        FOR (std::string const & rank, env.second->maxranks)
            std::cout << rank << ' ';
        std::cout << std::endl;
    }
}

// Format: max rank # > number limit    maxrank1 maxrank2 ...
void Problem::printranksinfo() const
{
    std::cout << maxranknumber << " < " << numberlimit << '\t';
    FOR (std::string const & rank, maxranks)
        std::cout << rank << ' ';
    std::cout << std::endl;
    printenvs();
}


// Move up to the parent. Return true if successful.
static bool moveup(pNode & p)
{
    if (!p.parent()) return false;

    p = p.parent();
    if (onlyopenchild(p) && p.parent() &&
        askyn("Go to grandparent y/n?"))
        p = p.parent();

    return true;
}

// Input the index and move to the child. Return true if successful.
static bool gototheirchild(pNode & p)
{
    std::size_t i;
    std::cin >> i;

    Problem::Children const & children(*p.children());
    return i >= children.size() ? pNode() : (p = children[i]);
}

// Move to the child with given assertion and index. Return true if successful.
static bool findourchild(pNode & p, strview ass, std::size_t index)
{
    FOR (pNode child, *p.children())
    {
        Move const & move = child->game().attempt;
        if (move.type == Move::THM && move.thmlabel() == ass && index-- == 0)
            return p = child;
    }
    return false;
}

// Input the assertion and the index and move to the child. Return true if successful.
static bool gotoourchild(pNode & p)
{
    if (!p.haschild())
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

    pNode grandchild = p;
    if (gotoonlyopenchild(grandchild) &&
        askyn("Go to only open child y/n?"))
        p = grandchild;
    return true;
}

void Problem::navigate(pNode p, bool detailed) const
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
    if (proof().empty() || !filename)
        return;

    Printer printer(&database.typecodes());
    verify(proof(), printer);

    std::ofstream out(filename);
    out << printer.str(indentations(ast(proof())));
}
