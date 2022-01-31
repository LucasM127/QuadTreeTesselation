#include "Interval.hpp"
#include <iostream>

void printOut(const std::vector<Intervali> &intervals)
{
    std::cout<<"Intervals held:\n";
    for(auto &I : intervals)
        std::cout<<"("<<I.min<<"-"<<I.max<<") : "<<I.val<<"\n";
}

void testA()
{
    Intervali A(0.f,4.f,1);
    Intervali B(0.0f,1.0f,2);
    Intervali C(1.0f,2.0f,3);
    Intervali D(2.0f,3.0f,4);
    Intervali E(3.0f,4.0f,5);

    std::vector<Intervali> intervals = {A};

    printOut(intervals);

    insert(B,intervals);
    insert(C,intervals);
    insert(D,intervals);
    insert(E,intervals);

    printOut(intervals);

    std::vector<Intervali> subRange = queryInterval(0.0f,1.0f,intervals);
    printOut(subRange);
    subRange = queryInterval(1.0f,2.0f,intervals);
    printOut(subRange);
    subRange = queryInterval(2.0f,3.0f,intervals);
    printOut(subRange);
    subRange = queryInterval(3.0f,4.0f,intervals);
    printOut(subRange);
}

void testB()
{
    Intervali A(0.f,4.f,1);
    Intervali B(0.5f,1.5f,2);
    Intervali C(1.5f,2.5f,3);
    Intervali D(2.5f,3.5f,4);

    std::vector<Intervali> intervals = {A};

    printOut(intervals);

    insert(B,intervals);
    insert(C,intervals);
    insert(D,intervals);

    printOut(intervals);

    std::vector<Intervali> subRange = queryInterval(0.0f,1.0f,intervals);
    printOut(subRange);
    subRange = queryInterval(1.0f,2.0f,intervals);
    printOut(subRange);
    subRange = queryInterval(2.0f,3.0f,intervals);
    printOut(subRange);
    subRange = queryInterval(3.0f,4.0f,intervals);
    printOut(subRange);
}

void testC()//made a wrong assumption about how the intervals work :(
{
    Intervali A(0.f,4.f,0);
    Intervali B(2.75f,3.5f,0);
    Intervali C(2.f,4.f,1);

    std::vector<Intervali> intervals = {A};

    printOut(intervals);

    insert(B,intervals);
    insert(C,intervals);

    printOut(intervals);

    std::vector<Intervali> subRange = queryInterval(0.0f,1.0f,intervals);
    printOut(subRange);
    subRange = queryInterval(1.0f,2.0f,intervals);
    printOut(subRange);
    subRange = queryInterval(2.0f,3.0f,intervals);
    printOut(subRange);
    subRange = queryInterval(3.0f,4.0f,intervals);
    printOut(subRange);
}

int main()
{
//    testA();//should only hold 1 each
//    std::cout<<"///////////////////\n";
//    testB();//should hold both...

    testC();

    return 0;
}