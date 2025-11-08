#ifndef WRITE_H_INCLUDED
#define WRITE_H_INCLUDED

#include "../types.h"

// Write the proof from pointers to proof of hypotheses. Return true if Okay.
bool writeproof(Proofsteps & dest, Assptr pthm, pProofs const & hyps);

#endif // WRITE_H_INCLUDED
