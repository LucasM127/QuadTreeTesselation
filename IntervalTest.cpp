#include "Interval.hpp"
#include <iostream>

void printOut(const std::vector<Intervali> &intervals)
{
    std::cout<<"Intervals held:\n";
    for(auto &I : intervals)
        std::cout<<"("<<I.min<<"-"<<I.max<<") : "<<I.val<<"\n";
}

int main()
{
    Intervali A(0.f,4.f,1);
    Intervali B(0.3f,2.2f,2);
    Intervali C(0.6f,0.9f,3);
    Intervali D(2.2f,2.8f,4);

    std::vector<Intervali> intervals = {A};

    printOut(intervals);

    insert(B,intervals);
    insert(C,intervals);
    insert(D,intervals);

    printOut(intervals);

    std::vector<Intervali> subRange = queryInterval(2.5f,4.f,intervals);
    printOut(subRange);

    return 0;
}