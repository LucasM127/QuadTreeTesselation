#include "Interval.hpp"

<<<<<<< HEAD
void insert(Intervali &I, std::vector<Intervali> &intervals)
=======
void insert(Intervali I, std::vector<Intervali>& intervals)
>>>>>>> cd5044f51de5c9640eda77621383aa070d2e1231
{
    auto it = intervals.begin();
    while(it != intervals.end())
    {
        if(it->contains(I))//it->min <= I.min)//
        {
            //create three ranges vs the one range
            //insert inserts before current iterator position and returns it to be the inserted position... ??
            Intervali A(it->min,I.min,it->val);
            Intervali Z(I.max,it->max,it->val);
            if(A.length() != 0.f)
            {
                it = intervals.insert(it,A);
                it = std::next(it);
            }
            *it = I;
            if(Z.length() != 0.f)
            {
                it = std::next(it);
                intervals.insert(it,Z);
            }
            return;
        }
        it = std::next(it);
    }
}

//assumes is in range else may go out of bounds but that is known for the use case
std::vector<Intervali> queryInterval(const float min, const float max, const std::vector<Intervali> &intervals)
{
    assert(min >= intervals.front().min && max <= intervals.back().max);

    auto it = intervals.begin();

    while(!it->contains(min))
    {
        it = std::next(it);
    }
    //it = std::prev(it);
    auto start = it;

    while(!it->contains(max))
        it = std::next(it);
    auto end = std::next(it);

    return std::vector<Intervali>(start,end);
}