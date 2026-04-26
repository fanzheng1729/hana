#include <cctype>   // for std::tolower
#include <fstream>
#include "../../io.h"
#include "../problem.h"
#include "../../proof/analyze.h"
#include "../../proof/printer.h"
#include "../../proof/verify.h"

// Return the only open child of p.
// Return nullptr if p has 0 or at least 2 open children.
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

// Format: ax-mp[!] or DEFER(n) or CONJ[!]
static void printattempt(pNode p)
{
    switch (p->game().attempt.type)
    {
    case Move::DEFER:
        std::cout << "DEFER(" << p->game().nDefer << ')';
        break;
    case Move::THM:
        std::cout << p->game().attempt.thmlabel();
        if (onlyopenchild(p)) std::cout << '!';
        break;
    case Move::CONJ:
        std::cout << "CONJ";
        if (onlyopenchild(p)) std::cout << '!';
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

// Format: ax-mp[!] or DEFER(n) or CONJ[!] score*size=UCB ...
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

static const char strproven[] = "V";

// Format: [(n)] maj score*size   [V]|- ...
static void printournode(pNode p, stage_t stage)
{
    Move const & lastmove = p.parent()->game().attempt;
    printstage(stage);
    std::cout << lastmove.subgoallabel(lastmove.matchsubgoal(p->game().goal()));
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
        while (p->game().attempt.subgoalfloats(i)) ++i;
        std::cout << p->game().attempt.subgoallabel(i++);
        printeval(child);
        std::cout << ' ';
    }
    std::cout << std::endl;
}

// Format: ax-mp[!] or CONJ score*size score*size
static void printtheirnode(pNode p)
{
    printattempt(p);
    std::cout << ' ';
    printhypsline(p);
}

// Format:
// Goal score [V]|- ...
// [Hyps hyp1 hyp2]
static void printgoal(pNode p)
{
    std::cout << "Goal " << Problem::value(p) << ' ';
    std::cout << &strproven[!p->game().proven()];
    std::cout << p->game().goal().expression();

    Assertion const & ass = p->game().env().assertion;
    if (ass.nEhyps() == 0) return;

    std::cout << "Hyps ";
    for (Hypsize i = 0; i < ass.nhyps(); ++i)
        if (!ass.hypfloats(i))
            std::cout << ass.hyplabel(i) << ' ';
    std::cout << std::endl;
}

// Format:
/**
 | Goal score |- ...
 | [Hyps hyp1 hyp2]
 | ax-mp score*size=UCB ... OR
 | ax-mp[!] or CONJ score*size score*size OR
 | DEFER(n) score*size
**/
// n.   (n) ax-mp[!] or CONJ min score*size maj score*size
//      maj score*size  |- ...
void Problem::printmainline(pNode p, size_type detail) const
{
    printgoal(p);
    // Print children.
    static void (*const printfn[])(pNode) = {&printtheirnode, &printdeferline};
    if (detail)
        if (isourturn(p))
            printourchildren(p, *this);
        else
            (*printfn[p->game().attempt.isdefer()])(p),
            p->game().attempt.printconj();
// std::cout << "Children printed" << std::endl;
    size_type level = 0, nDefer = 0;
    while (p = pickchild(p))
        if (!isourturn(p))
            switch (p->game().attempt.type)
            {
            case Move::DEFER:
                ++nDefer;
                continue;
            case Move::THM: case Move::CONJ:
                std::cout << ++level << ".\t";
                printstage(nDefer);
                nDefer = 0;
                printtheirnode(p);
                continue;
            case Move::NONE:
                std::cerr << "Empty move" << std::endl;
                return;
            }
        // Our turn
        else if (p->game().nDefer == 0)
        {
            std::cout << '\t';
            printournode(p, p->stage() * staged);
            if (detail > level)
                printourchildren(p, *this);
        }
}

void Problem::checkmainline(pNode p) const
{
    for ( ; p; p = pickchild(p))
        if (isourturn(p) && p->game().nDefer == 0)
            if (p->game().proven() != p->won())
            {
                for (pNode q = root(); q && q != p; q = pickchild(q))
                    std::cout << *q;
                std::cout << *p;
                navigate();
            }
}

// Format: n nodes, x V, y ?, z X in m contexts
void Problem::printstats() const
{
    std::cout << nplays() << " plays, " << size() << " nodes, ";
    std::cout << nProof() << '/';
    static const char * const s[] = {" V, ", " ?, ", " X in "};
    for (int i = GOALTRUE; i >= GOALFALSE; --i)
        std::cout << nGoal(i) << s[GOALTRUE - i];
    std::cout << nEnvs();
    std::cout << '(' << nsubEnvs() << '/' << nsupEnvs() << ')';
    std::cout << " contexts" << std::endl;
    unexpected(nGoal(GOALNEW) > 0, "unevaluated", "goal");
}

// Format: [*]label     maxrank1 maxrank2 ...
void Problem::printenvs() const
{
    FOR (Environs::const_reference env, environs)
    {
        SyntaxDAG::Ranks const & envranks = env.second->maxranks;
        char const c = " *"[env.second->rankssimplerthanProb()];
        std::cout << c << env.second->label << '\t';
        FOR (std::string const & rank, env.second->maxranks)
            std::cout << rank << ' ';
        std::cout << std::endl;
    }
}

// Format: max rank # < number limit    maxrank1 maxrank2 ...
void Problem::printranksinfo() const
{
    std::cout << maxranknumber << " < " << numberlimit << '\t';
    FOR (std::string const & rank, maxranks)
        std::cout << rank << ' ';
    std::cout << std::endl;
    printenvs();
}


// Move up to the parent. Return true if okay.
static bool moveup(pNode & p)
{
    if (!p.parent()) return false;

    p = p.parent();
    if (p.parent())
        if (p.nchild() == 1 ||
            (onlyopenchild(p) &&
            askyn("Go to grandparent y/n?")))
            p = p.parent();

    return true;
}

// Input the index and move to the child. Return true if okay.
static bool gototheirchild(pNode & p)
{
    std::size_t i;
    std::cin >> i;

    Problem::Children const & children(*p.children());
    return i >= children.size() ? pNode() : (p = children[i]);
}

// Move to the child with given assertion and index. Return true if okay.
static bool findourchild(pNode & p, strview token, std::size_t index)
{
    FOR (pNode child, *p.children())
    {
        Move const & move = child->game().attempt;
        if (move.label() == token && index-- == 0)
            return p = child;
    }
    return false;
}

// Ask the user for going to the only open child.
static void askonlyopenchild(pNode & p)
{
    pNode child = p;
    if (gotoonlyopenchild(child) &&
        askyn("Go to only open child y/n?"))
        p = child;
}

// Input the assertion and the index and move to the child. Return true if okay.
static bool gotoourchild(pNode & p)
{
    if (!p.haschild())
        return std::cout << "No child" << std::endl, false;

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

    if (!Problem::isourturn(p) && p.nchild() == 1)
        p = p.children()->front();
    else
        askonlyopenchild(p);
    return true;
}

// Go down a given # of levels.
void Problem::gotolevel(pNode & p, size_type level) const
{
    if (!p || level == 0)
        return;

    pNode q;
    size_type n = 0;
    while (q = pickchild(p))
        if (!isourturn(p = q))
            switch (p->game().attempt.type)
            {
            case Move::DEFER:
                continue;
            case Move::THM: case Move::CONJ:
                if (++n == level)
                    return askonlyopenchild(p);
                continue;
            case Move::NONE:
                std::cerr << "Empty move" << std::endl;
                return;
            }
}

// Go down a # of levels read from input.
void Problem::gotolevel(pNode & p) const
{
    size_type n;
    std::cin>>n;
    gotolevel(p, n);
}

void Problem::navigate(pNode p, bool detailed) const
{
    if (!p) return;

    std::cout <<
    "c|g label n -> n-th node w/label, c|g * -> last node\n"
    "c|g n -> n-th hypothesis\n"
    "d(ive) or j(ump) n -> the n-th level\n"
    "u(p) -> parent, uu -> the closest non-defer parent\n"
    "h(ome) or t(op) -> the top level\n"
    "b(ye) or e(xit) or q(uit) to end navigation\n"
    "a(ssertion) or n(umber) or # for the assertion number\n";

    std::string token;
    while (true)
    {
// std::cout << isourturn(node) << ' ' << &node.value() << std::endl;
        std::cin.clear();
        std::cout << "> ";

        std::cin >> token;
        switch (std::tolower(static_cast<unsigned char>(token[0])))
        {
        case 'b': case 'e': case 'q':
            return;
        case 'd': case 'j':
            gotolevel(p);
            break;
        case 'u':
            moveup(p);
            if (token[1] == 'u')
                while (p->game().nDefer)
                    moveup(p);
            break;
        case 'h': case 't':
            p = root();
            break;
        case 'c': case 'g':
            isourturn(p) ? gotoourchild(p) : gototheirchild(p);
            break;
        case 'a': case 'n': case '#':
            std::cout << number() << std::endl;
            continue;
        default:
            unexpected(true, "command", token);
            continue;
        }
        printmainline(p, detailed);
    }

    static std::string const hline(50, '-');
    std::cout << hline << std::endl;
}

void Problem::printabs() const
{
    FOR (Abstractions::const_reference absRPN, abstractions)
        std::cout << verify(absRPN.first);
}

void Problem::writeproof(const char * const filename) const
{
    if (proof().empty() || !filename)
        return;

    Printer printer(&database().typecodes());
    verify(proof(), printer);

    std::ofstream out(filename);

    if (out)
        out << printer.str(indentations(ast(proof())));
}
