#ifndef STAGEVAL_H_INCLUDED
#define STAGEVAL_H_INCLUDED

#include <cstddef> // for std::size_t

// Stage
typedef std::size_t stage_t;
// Value
typedef double Value;

// Predefined game values
enum    WDL { LOSS = -1, DRAW = 0, WIN = 1 };
inline  bool issure(Value v)
    { return v == WDL::WIN || v == WDL::LOSS; }

// Evaluation
struct  Eval
{
    Value value;
    bool sure;
    Eval(Value v, bool issure) : value(v), sure(issure) {}
    Eval(Value v = 0) : value(v), sure(issure(v)) {}
};
inline  bool operator==(Eval x, Eval y)
    { return x.value == y.value && x.sure == y.sure; }
inline  bool operator!=(Eval x, Eval y)
    { return !(x == y); }

// Position lost
static  Eval const EvalLOSS  = Eval(WDL::LOSS,  true);
// Position drawn
static  Eval const EvalDRAW  = Eval(WDL::DRAW,  true);
// Position won
static  Eval const EvalWIN   = Eval(WDL::WIN,   true);

#endif // STAGEVAL_H_INCLUDED
