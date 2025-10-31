#ifndef RELATION_H_INCLUDED
#define RELATION_H_INCLUDED

#include <map>
#include <vector>
#include "types.h"
#include "util/for.h"

static const unsigned NJUSTIFICATION = 16;
struct Justifications
{
    // Pointers to justifications of the relation
    Assptr data[NJUSTIFICATION];
    // Pointer to the syntaxiom being justified
    Assptr pass;
    Assptr & operator[](unsigned i) { return data[i]; }
    Assptr const & operator[](unsigned i) const { return data[i]; }
    Reltype type() const
    {
        Reltype result = 0;
        for (Assptr const * p = &data[NJUSTIFICATION - 1]; p >= data; --p)
            result = 2 * result + !!*p;
        return result;
    }
};
// Map: equality syntax axioms -> its justifications
struct Relations : std::map<strview, Justifications>
{
    enum
    {
        PATTERN_END = -2,
        LINE_END    = -1,
        REFLEXIVITY = 1 << 0,
        SYMMETRY    = 1 << 1,
        TRANSITIVITY= 1 << 2,
        PARTIALORDER= REFLEXIVITY + TRANSITIVITY,
        EQUIVALENCE = PARTIALORDER + SYMMETRY,
        AX1 = 1 << 3,
        ID1 = 1 << 4,
        ID2 = 1 << 5,
        ID12= ID1 + ID2,
        // NOT satisfies ID2 but not ID1; PROVABLE satisfies BOTH.
        ANL = 1 << 6,
        ANR = 1 << 7,
        AND = ANL + ANR,
        ORL = 1 << 8,
        ORR = 1 << 9,
        OR  = ORL + ORR,
        AN1 = 1 << 10,
        AN2 = 1 << 11,
        AN3 = 1 << 12,
        A3AN= AN1 + AN2 + AN3,
        OR1 = 1 << 13,
        OR2 = 1 << 14,
        OR3 = 1 << 15,
        O3OR= OR1 + OR2 + OR3,
        TYPE_COUNT  = 1 << NJUSTIFICATION
    };
    // TRU is not listed here,
    // because it is syntactically indistinguishable from CHOICE.
    // Its truth table must be derivaed otherwise.
    static const int (&patterns())[NJUSTIFICATION][12]
    {
        static const int a[NJUSTIFICATION][12] =
        {
            {1, 1, 0, PATTERN_END}, // Reflexivity
            {1, 2, 0, LINE_END, 2, 1, 0, PATTERN_END}, // Symmetry
            {1, 2, 0, LINE_END, 2, 3, 0, LINE_END, 1, 3, 0, PATTERN_END}, // Transitivity
            {1, 2, 1, 0, 0, PATTERN_END}, // ax1: P -> (Q -> P)
            {1, LINE_END, 1, 0, PATTERN_END}, // id1: once idempotent
            {1, LINE_END, 1, 0, 0, PATTERN_END}, // id2: twice idempotent
            {1, 2, 0, LINE_END, 1, PATTERN_END}, // anl: P /\ Q => P
            {1, 2, 0, LINE_END, 2, PATTERN_END}, // anr: P /\ Q => Q
            {1, LINE_END, 1, 2, 0, PATTERN_END}, // orl: P => P \/ Q
            {2, LINE_END, 1, 2, 0, PATTERN_END}, // orr: Q => P \/ Q
            {1, 2, 3, 0, LINE_END, 1, PATTERN_END}, // anl: P /\ Q /\ R => P
            {1, 2, 3, 0, LINE_END, 2, PATTERN_END}, // an2: P /\ Q /\ R => Q
            {1, 2, 3, 0, LINE_END, 3, PATTERN_END}, // an3: P /\ Q /\ R => R
            {1, LINE_END, 1, 2, 3, 0, PATTERN_END}, // orl: P => P \/ Q \/ R
            {2, LINE_END, 1, 2, 3, 0, PATTERN_END}, // or2: Q => P \/ Q \/ R
            {3, LINE_END, 1, 2, 3, 0, PATTERN_END}, // or3: R => P \/ Q \/ R
        };
        return a;
    }
    Relations() {}
// Find relations and their justifications among syntax axioms.
    Relations(Assertions const & assertions);
// Return relations of a given type.
    Relations operator()(Reltype type) const
    {
        Relations result;
        FOR (const_reference relation, *this)
            if (relation.second.type() == type)
                result.insert(relation);
        return result;
    }
};

#endif // RELATION_H_INCLUDED
