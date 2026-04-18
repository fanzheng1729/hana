#ifndef SAT_H_INCLUDED
#define SAT_H_INCLUDED

#include <limits>
#include "../CNF.h"

struct SATsolver
{
    CNFClauses const & cnf, & cnf2;
    Atom const cnfatoms, cnf2atoms;
    SATsolver
        (CNFClauses const & hyps, CNFClauses const & morehyps = CNFClauses()) :
        cnf(hyps), cnf2(morehyps),
        cnfatoms(cnf.natoms()), cnf2atoms(cnf2.natoms()) {}
    // Map: free atoms -> truth value.
    // Return the empty vector if unsuccessful.
    Bvector truthtable(Atom nfree)
    {
        static Atom const maxnatoms = std::numeric_limits<Atom>::digits;
        if (nfree > maxnatoms) nfree = maxnatoms;
        if (nfree > cnfatoms) nfree = cnfatoms;

        Bvector tt(static_cast<TTindex>(1) << nfree, false);
        if (cnf.hasemptyclause()) return tt;

        CNFClauses excnf(cnf);
        for (TTindex arg = 0; arg < tt.size(); ++arg)
        {
            for (Atom i = 0; i < nfree; ++i)
                excnf.push_back(CNFClause(1, i * 2 + 1 - (arg >> i & 1)));
            tt[arg] = excnf.sat();
            excnf.resize(cnf.size());
        }
        return tt;
    }
    // Return true if the SAT instance is satisfiable.
    // Reference backtracking solver
    bool sat()
    {
        // Initial model
        CNFModel model(std::max(cnfatoms, cnf2atoms));
        // Current atom being assigned
        Atom atom = 0;

        while (true)
        {
            switch (model[atom])
            {
    //std::cout << "Trying atom " << atom << " = " << model[atom] << '\n';
            case UNKNOWN : case FALSE :
                ++model[atom];
                // Check if there is a contradiction so far.
                if (cnf.okaysofar(model) && cnf2.okaysofar(model))
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
            case TRUE:
                // Un-assign the current atom.
                do
                {
                    model[atom] = UNKNOWN;
                    if (atom == 0)
                        // All models tried
                        return false;
                } while (model[--atom] == TRUE);
            }
        }
    }
};

#endif // SAT_H_INCLUDED
