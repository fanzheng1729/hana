#include <deque>
#include "cnf.h"
#include "io.h"
#include "util/arith.h"

std::ostream & operator<<(std::ostream & out, const CNFClauses & cnf)
{
    FOR (CNFClause const & clause, cnf)
        out << clause;
    return out;
}

// Append cnf to the end.
// If atom < argcount, change it to arglist[atom], with sense adjusted.
// If atom >= argcount, change it to new atoms starting from atomcount.
// argcount and arglist are separate to work with stack based arguments.
void CNFClauses::append
    (CNFClauses const & cnf, Atom const atomcount,
     Literal const * const arglist, Atom const argcount)
{
    size_type const oldsize = size();
    resize(oldsize + cnf.size());
    // New clauses
    for (size_type i = 0; i < cnf.size(); ++i)
    {
        (*this)[oldsize + i].resize(cnf[i].size());
        for (CNFClause::size_type j = 0; j < cnf[i].size(); ++j)
        {
            Literal const lit = cnf[i][j];
            (*this)[oldsize + i][j] = lit / 2 < argcount ?
            arglist[lit / 2] ^ (lit & 1) : lit + (atomcount - argcount) * 2;
        }
    }
}

// Check the satisfaction of clause under the model.
// If UNIT, return (UNIT, index of unassigned literal).
// If UNDECIDED, return (UNDECIDED, index of unassigned literal).
std::pair<CNFClausesat, CNFClause::size_type> CNFclausesat
        (CNFClause const & clause, CNFModel const & model)
{
    bool nonefound = false;
    Atom oldatom = 0;
    CNFClause::size_type unitindex = 0;

    FOR (Literal const & lit, clause)
    {
        switch (model.test(lit))
        {
        case CNFNONE:
            if (nonefound && lit / 2 != oldatom)
                // both atoms are NONE
                return std::make_pair(UNDECIDED, unitindex);
            nonefound = true;
            oldatom = lit / 2;
            unitindex = &lit - clause.data();
            continue;
        case CNFTRUE:
            return std::make_pair(SATISFIED, 0u);
        case CNFFALSE:
            ;
        }
    }

    return std::make_pair(nonefound ? UNIT : CONTRADICTORY, unitindex);
}

typedef std::size_t Mask;

static bool checkmask
    (Bvector const & truthtable,
        Atom atomcount, TTindex newindex, Mask mask,
     Bvector & compare)
{
    for (Atom i = 0; i < atomcount; ++i)
    {
        if (!(mask >> i & 1)) // mask[i] = 0
            continue;
        if (!compare[newindex ^ static_cast<TTindex>(1) << i])
            return false;
    }
    compare[newindex] = (truthtable[newindex] == truthtable[newindex ^ mask]);
    return compare[newindex];
}

static void addclausefromindexmask
    (Atom atomcount, TTindex index, bool value, Mask mask, CNFClauses & cnf)
{
    cnf.resize(cnf.size() + 1);
    CNFClause & clause = cnf.back();

    for (Atom i = 0; i < atomcount; ++i)
    {
        if (mask >> i & 1) // mask[i] = 1
            continue;
        // mask[i] = 0. bit = index[i].
        bool const bit = index >> i & 1;
        // Add i if i is false, ~i if i is true.
        clause.push_back(i * 2 + bit);
    }

    // Add positive lit if value is true, negative lit if false
    clause.push_back(atomcount * 2 + (value ^ 1));
}

// Return a clause covering truthtable[index], and update processed.
static void processtruthtableentry
    (Bvector const & truthtable, TTindex index,
     Bvector & processed, CNFClauses & cnf)
{
    Atom const atomcount = util::log2(truthtable.size());
    Bvector maskadded, compare;
    maskadded.assign(truthtable.size(), false);
    // compare[i] = if there is a block containing index and i
    compare = maskadded;
    compare[index] = true;

    std::deque<Mask> masks;
    masks.assign(1, 0);
    while (!masks.empty())
    {
        // Read the current mask.
        Mask const mask = masks[0];
        masks.pop_front();
//std::cout << "Checking mask " << mask << std::endl;
        bool newmaskfound = false;
        for (Atom imask = 0; imask < atomcount; ++imask)
        {
            Mask const newmask = mask | static_cast<Mask>(1) << imask;
            if (newmask == mask)
                continue; // mask[imask] = 1
            // Check if bit i can be added to mask.
//std::cout << mask << ".flip(" << imask << ") = " << newmask << std::endl;
            TTindex const newindex = index ^ newmask;
            // Check the new mask.
            if (checkmask(truthtable, atomcount, newindex, newmask, compare))
            {
//std::cout << "Add mask " << newmask << std::endl;
                masks.push_back(newmask);
                newmaskfound = true;
                processed[newindex] = true;
            }
        }
        if (!newmaskfound && !maskadded[mask])
        {
//std::cout << "Add clause from mask " << mask << std::endl;
            maskadded[mask] = true;
            addclausefromindexmask(atomcount,index,truthtable[index],mask,cnf);
        }
    }
}

// Construct the cnf representing a truth table.
CNFClauses::CNFClauses(Bvector const & truthtable)
{
    Bvector processed(truthtable.size(), false);
    Bvector::iterator const begin(processed.begin());
    for (TTindex i = 0; i < truthtable.size();)
    {
        processtruthtableentry(truthtable, i, processed, *this);
        i = std::find(begin + i + 1, processed.end(), false) - begin;
    }
}
