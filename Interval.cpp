#include "Interval.hpp"

void insert(Intervali &I, std::vector<Intervali> &intervals)
{
    auto it = intervals.begin();
    while(it != intervals.end())
    {
        if(it->contains(I))
        {
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
        else if(I.contains(*it))
        {
            Intervali A(I.min,it->min,I.val);
            Intervali Z(it->max,I.max,I.val);
            if(A.length() != 0.f)
            {
                it = intervals.insert(it,A);
                it = std::next(it);
            }
//            *it = I;
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

std::vector<Intervali> queryInterval(const float min, const float max, const std::vector<Intervali> &intervals)
{
    assert(min >= intervals.front().min && max <= intervals.back().max);

    std::vector<Intervali>::const_iterator start, end;
    std::vector<Intervali>::const_iterator it = intervals.begin();
    while(it != intervals.end())
    {
        if(it->contains(min))
        {
            break;
        }
        it = std::next(it);
    }
    if(it->max == min)
        it = std::next(it);
    start = it;
    while(it != intervals.end())
    {
        if(it->min >= max)
        {
            break;
        }
        it = std::next(it);
    }
    end = it;

    return std::vector<Intervali>(start,end);
}