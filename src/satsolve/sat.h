#ifndef SAT_H_INCLUDED
#define SAT_H_INCLUDED

#include <limits>
#include "../cnf.h"

struct Satsolver
{
    CNFClauses const & rcnf;
    Satsolver(CNFClauses const & cnf) : rcnf(cnf) {}
    // Map: free atoms -> truth value.
    // Return the empty vector if not okay.
    Bvector truthtable(Atom nfree)
    {
        static Atom const maxnatom = std::numeric_limits<Atom>::digits;
        if (nfree > maxnatom) nfree = maxnatom;
        Atom const atomcount = rcnf.atomcount();
        if (nfree > atomcount) nfree = atomcount;

        Bvector tt(static_cast<TTindex>(1) << nfree, false);
        if (rcnf.hasemptyclause()) return tt;

        CNFClauses cnf2(rcnf);
        for (TTindex arg = 0; arg < tt.size(); ++arg)
        {
            for (Atom i = 0; i < nfree; ++i)
                cnf2.push_back(CNFClause(1, i * 2 + 1 - (arg >> i & 1)));
            tt[arg] = cnf2.sat();
            cnf2.resize(rcnf.size());
        }
        return tt;
    }
    // Return true if the SAT instance is satisfiable.
    // Reference backtracking solver
    bool sat()
    {
        // Initial model
        CNFModel model(rcnf.atomcount());
        // Current atom being assigned
        Atom atom = 0;

        while (true)
        {
            switch (model[atom])
            {
    //std::cout << "Trying atom " << atom << " = " << model[atom] << '\n';
            case CNFNONE : case CNFFALSE :
                ++model[atom];
                // Check if there is a contradiction so far.
                if (rcnf.okaysofar(model))
                {
                    // No contradiction yet. Move to next atom.
                    if (++atom == model.size())
                    {
                        // All atoms assigned
                        //std::cout << model;
                        return true;
                    }
                }
                // Move to next model.
                continue;
            case CNFTRUE:
                // Un-assign the current atom.
                do
                {
                    model[atom] = CNFNONE;
                    if (atom == 0)
                        // All models tried
                        return false;
                } while (model[--atom] == CNFTRUE);
            }
        }
    }
};

#endif // SAT_H_INCLUDED
