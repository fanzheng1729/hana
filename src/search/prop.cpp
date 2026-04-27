#include "../disjvars.h"
#include "goaldata.h"
#include "../io.h"
#include "../param.h"
#include "problem.h"
#include "prop.h"
#include "../util/progress.h"
#include "../util/timer.h"

// Adds substitutions to a move.
struct Substadder : Adder
{
    Expression const & freevars;
    Moves & moves;
    Move & move;
    Environ const & env;
    Substadder
    (Expression const & freevars, Moves & moves, Move & move,
        Environ const & env) :
        freevars(freevars), moves(moves), move(move), env(env) {}
    // Add a move. Return true if the move closed the goal.
    bool operator()(Argtypes const & types, Genresult const & result,
                    Genstack const & stack)
    {
        for (RPNsize i = 0; i < types.size(); ++i)
            move.substitutions[freevars[i]] =
            result.at(freevars[i].typecode())[stack[i]];
        // Filter move by SAT.
        switch (env.valid(move))
        {
        case Environ::MoveCLOSED:
            moves.assign(1, move);
            return true;
        case Environ::MoveVALID:
            // std::cout << move << std::endl;
            // std::cout << move.substitutions;
            moves.push_back(move);
            // std::cin.get();
            return false;
        default:
            return false;
        }
    }
};

// Add a move with free variables. Return false.
bool Prop::addhardmoves(Move & move, RPNsize size, Moves & moves) const
{
// if (size == 3)
// std::cout << syntaxioms, std::cin.get();
    pAss pthm = move.pthm;
// if (size >= 5)
// std::cout << "Trying " << pthm->first << ' ' << size << std::endl;
    Assertion const & thm = pthm->second;
    // Free variables in the theorem to be used
    Expression freevars;
    // Type codes of free variables
    Argtypes types;
    // Preallocate for efficiency.
    freevars.reserve(thm.nfreevar()), types.reserve(thm.nfreevar());
    FOR (Varusage::const_reference var, thm.varusage)
        if (!var.second.back())
            freevars.push_back(var.first), types.push_back(var.first.typecode());
    // Generate substitution terms.
    FOR (Symbol3 var, freevars)
        generateupto(var.typecode(), size);
    // Generate substitutions.
    Substadder adder(freevars, moves, move, *this);
    dogenerate(types, size + 1, adder);
// if (size >= 5)
// std::cout << moves.size() << " hard moves found" << std::endl;
    return false;
}

static void printtime(Treesize nodes, Time time)
{
    std::cout << nodes << " nodes / " << time << "s = ";
    std::cout << nodes/time << " nps" << std::endl;
}

// Test propositional proof search. Return true if okay.
bool testpropsearch
    (Database const & database, Param const & param)
{
    std::cout << "Testing propositional proof search";
    Progress progress(std::cerr);
    Timer timer;
    bool okay = true;
    Treesize nodes = 0;
    nAss allprop = 0, proven = 0;
    // Test assertions
    Assiters const & assiters = database.assiters();
    nAss const all = assiters.size();
    for (nAss i = 1; i < all; ++i)
    // for (nAss i = 1658; i < 1659; ++i)
    {
        Assiter const iter = assiters[i];
        Assertion const & ass = iter->second;

        // printass(*iter);
        // std::cout << profile(ass.expRPNAST());
        // std::cin.get();

        // Skip axioms, trivial and duplicate theorems.
        if (ass.testtype(Asstype::AXIOM + Asstype::DUPLICATE))
            continue;

        // Skip non propositional theorems.
        static const Prop prop1(ass, database.propctors(), 0, 0);
        if (!(prop1.ontopic(ass)))
            continue;

        // Try search proof.
        ++allprop;
        const   Prop prop(ass, database.propctors(), param.weightfactor, param.maxsize);
        const   MCTSParams v = {0, param.exploration};
        Problem tree(prop, database, v, param.staged);
        const   Treesize treesize = testsearch(iter, tree, param.maxsize);
        if (treesize == 0)
        {
            okay = false;
            break;
        }
        nodes   += treesize;
        proven  += treesize <= param.maxsize;
        progress << i/static_cast<Ratio>(all - 1);
    }

    // Print stats.
    std::cout << '\n';
    printtime(nodes, timer);
    printpercent(proven, "/", allprop, " = ", "% proven\n");
    return okay;
}
