#ifndef GETPROOF_H_INCLUDED
#define GETPROOF_H_INCLUDED

#include "types.h"

enum ReadStatus {INCOMPLETE = -1, PROOFBAD = 0, PROOFOKAY = 1};

struct Tokens;

// Determine if there is no more token before finishing a statement.
bool unfinishedstat(Tokens const & tokens, strview stattype, strview label);

// Print error message (if any) in the proof of label.
ReadStatus printprooferr(strview label, ReadStatus err);

// Get sequence of letters in a compressed proof (Appendix B).
ReadStatus getproofletters(strview label, Tokens & tokens, std::string & proof);

// Get the raw numbers from compressed proof format.
// The letter Z is translated as 0 (Appendix B).
Proofnumbers getproofnumbers(strview label, std::string const & proof);

#endif // GETPROOF_H_INCLUDED
