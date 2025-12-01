#ifndef PROGRESS_H_INCLUDED
#define PROGRESS_H_INCLUDED

#include <iostream>

typedef double Ratio;
template<class T, class U> double ratio(T x, U y)
    { return static_cast<double>(x)/static_cast<double>(y); }

struct Progress
{
    Progress(std::ostream & out = std::cout) : count(0), cout(out)
    {
        cout <<
        "\n0%...10...20...30...40...50...60...70...80...90...|" << std::endl;
    }
    void operator<<(Ratio progress)
    {
        if (progress > 1) progress = 1;
        for ( ; count < static_cast<int>(progress * 50) + 1; ++count)
            cout << '<' << std::flush;
        if (progress == 1) cout << std::endl;
    }
private:
    int count;
    std::ostream & cout;
};

#endif // PROGRESS_H_INCLUDED
