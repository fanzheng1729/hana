#include "../CNF.h"
#include "../disjvars.h"
#include "goaldata.h"
#include "../io.h"
#include "problem.h"
#include "prop.h"
#include "../util/progress.h"
#include "../util/timer.h"

// Determine status of a goal.
Goalstatus Prop::status(Goal const & goal) const
{
    goal.fillast();

    CNFClauses conclusion;
    // Add Conclusion.
    Atom natom = hypnatoms;
    if (!propctors().addformula
        (goal.RPN, goal.ast, assertion.hypiters, conclusion, natom))
        return statuserr(*this, goal.RPN);
    // Negate conclusion.
    return hypscnf.first.sat(conclusion.closeoff(natom - 1, true)) ?
            GOALFALSE : GOALTRUE;
}

// Return the hypotheses of a goal to trim.
Bvector Prop::hypstotrim(Goal const & goal) const
{
    Hypsize const nhyps = assertion.nhyps();
    Bvector result(nhyps, false);

    bool trimmed = false;
    for (Hypsize i = nhyps - 1; i != static_cast<Hypsize>(-1); --i)
    {
        if (assertion.hypfloats(i)) continue;
// std::cout << "Trimming hypothesis " << assertion.hyplabel(i) << std::endl;
        result[i] = true;
        // Check if it can be trimmed.
        CNFClauses cnf;
        Proofnumbers const & ends = hypscnf.second;
        // Add hypotheses.
        for (Hypsize j = 0; j < nhyps; ++j)
        {
            if (assertion.hypfloats(j) || result[j])
                continue; // Skip floating or trimmed hypotheses.
// std::cout << "Adding hypothesis " << assertion.hyplabel(j) << std::endl;
            // Add CNF clauses.
            CNFClauses::size_type begin = j>0 ? ends[j - 1] : 0, end = ends[j];
            CNFClause const * const hypsdata = hypscnf.first.data();
            cnf.insert(cnf.end(), hypsdata + begin, hypsdata + end);
        }
// std::cout << "hypcnf\n" << hypscnf.first << "cnf\n" << cnf;
        Atom natom = cnf.empty() ? nhyps : cnf.natoms();
        // Add conclusion.
        propctors().addformula
        (goal.RPN, goal.ast, assertion.hypiters, cnf, natom);
        // Negate conclusion.
        trimmed |= (result[i] = !cnf.closeoff(natom - 1, true).sat());
    }

    return trimmed ? assertion.trimvars(result, goal.RPN) : Bvector();
}

static double distance(Freqcounts const & goal, Frequencies const & ref)
{
    std::size_t const size = goal.size();
    if (unexpected(size != ref.size(), "size mismatch", ""))
        return 0;
    return 0;
    // Total occurrence count
    double const total = static_cast<double>(goal.sum());

    double dist = 0;
    for (std::size_t i = 0; i < size; ++i)
    {
        double  diff = goal[i]/total - ref[i];
        dist += diff * diff;
    }
    return dist;
}

// Evaluate leaf games, and record the proof if proven.
Eval Prop::evalourleaf(Game const & game) const
{
    Weight const w = this->Environ::weight(game) + game.wDefer();
    Freqcounts propctorcounts = hypspropctorcounts;
    addfreqcounts(game.goal().RPN, propctorlabels, propctorcounts);
    double const d = distance(propctorcounts, propctorfreqs);
    return score(w + d * frequencybias);
}

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
        for (Proofsize i = 0; i < types.size(); ++i)
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
bool Prop::addhardmoves
    (pAss pthm, Proofsize size, Move & move, Moves & moves) const
{
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
    FOR (Symbol3 const var, freevars)
        generateupto(var.typecode(), size);
    // Generate substitutions.
    Substadder adder(freevars, moves, move, *this);
    dogenerate(types, size + 1, adder);
// if (size >= 5)
// std::cout << moves.size() << " hard moves found" << std::endl;
    return false;
}

static void printtime(Problem::size_type nodes, Time time)
{
    std::cout << nodes << " nodes / " << time << "s = ";
    std::cout << nodes/time << " nps" << std::endl;
}

// Test propositional proof search. Return true if okay.
bool testpropsearch
    (Database const & database, std::size_t maxsize, double const parameters[4])
{
    std::cout << "Testing propositional proof search";
    Progress progress(std::cerr);
    Timer timer;
    bool okay = true;
    Problem::size_type nodes = 0;
    Assiters::size_type all = 0, proven = 0;
    // Test assertions
    Assiters const & assiters = database.assiters();
    for (Assiters::size_type i = 1; i < assiters.size(); ++i)
    {
        Assiter const iter = assiters[i];

        // Skip axioms, trivial and duplicate theorems.
        if (iter->second.type
            & (Asstype::AXIOM + Asstype::DUPLICATE))
            continue;

        // Skip non propositional theorems.
        static const Prop prop1(assiters[1]->second, database, 0, 0, false);
        if (!(prop1.ontopic(iter->second)))
            continue;

        // Try search proof.
        ++all;
        Prop prop(iter->second, database, maxsize, parameters[2], parameters[3]);
        Problem tree(prop, parameters);
        Problem::size_type const n = testsearch(iter, tree, maxsize);
        if (n == 0)
        {
            okay = false;
            break;
        }
        nodes += n;
        proven += n <= maxsize;
        progress << i/static_cast<Ratio>(assiters.size() - 1);
    }

    // Print stats.
    std::cout << '\n';
    printtime(nodes, timer);
    printpercent(proven, "/", all, " = ", "% proven\n");
    return okay;
}
