#ifndef PRINTER_H_INCLUDED
#define PRINTER_H_INCLUDED

#include "../types.h"

// Pretty printer of proof steps
struct Printer
{
    Printer(struct Typecodes const * p = NULL) : ptypes(p), savecount(0) {}
    // Check if the printer is on.
    operator bool() const { return ptypes; }
    // Add a proof step.
    bool addstep(Proofstep step, Proofsize index, Expression const & stacktop)
    { return !*this || doaddstep(step, index, stacktop); }
    // Return proof string for display.
    std::string str(std::vector<Proofsize> const & indentation) const;
private:
    struct Typecodes const * ptypes;
    // # saved steps
    std::vector<Expression>::size_type savecount;
    // Stack of essential steps
    std::vector<std::vector<Expression>::size_type> stack;
    // List of (justification, tag #, index, expression)
    std::vector<Expression> steps;
    // Add a proof step.
    bool doaddstep(Proofstep step, Proofsize index, Expression const & stacktop);
    bool addstep(Expression const & stacktop, Proofsize index,
                 strview label = "", Symbol2 save = Symbol2());
};

#endif // PRINTER_H_INCLUDED
