#ifndef STAGEVAL_H_INCLUDED
#define STAGEVAL_H_INCLUDED

#include <cstddef> // for std::size_t

typedef std::size_t stage_t;
typedef double Value;

// Predefined game values
enum    WDL { LOSS = -1, DRAW = 0, WIN = 1 };
static  const Value ALMOSTWIN = WDL::WIN - std::numeric_limits<Value>::epsilon();
static  const Value ALMOSTLOSS = -ALMOSTWIN;

// Return true if the game ends with value v.
inline  bool issure(Value v)
    { return v == WDL::WIN || v == WDL::LOSS; }

// Evaluation
struct  Eval
{
    Value value;
    bool sure;
    Eval(Value v, bool issure) : value(v), sure(issure) {}
    Eval(Value v = WDL::DRAW) : value(v), sure(issure(v)) {}
};
inline  bool operator==(Eval x, Eval y)
    { return x.value == y.value && x.sure == y.sure; }
inline  bool operator!=(Eval x, Eval y)
    { return !(x == y); }

// Predefined evaluations
static  Eval const EvalLOSS  = Eval(WDL::LOSS,  true);
static  Eval const EvalDRAW  = Eval(WDL::DRAW,  true);
static  Eval const EvalWIN   = Eval(WDL::WIN,   true);

#endif // STAGEVAL_H_INCLUDED
