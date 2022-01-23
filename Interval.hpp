#ifndef INTERVAL_HPP
#define INTERVAL_HPP

#include <cassert>
#include <vector>

//not a full fledged interval library but just for what I need
//Interval that holds an id ... intervali
struct Intervali
{
    Intervali(float a, float b, unsigned int i) : min(a), max(b), val(i)
    {
        assert(min <= max);
    }
    inline float length() const
    {
        return max - min;
    }
    inline bool contains(const Intervali &I) const
    {
        return I.max <= max && I.min >= min;
    }
    inline bool contains(const float f) const
    {
        return f >= min && f <= max;
    }
    float min, max;
    unsigned int val;
};

void insert(Intervali &I, std::vector<Intervali> &intervals);

std::vector<Intervali> queryInterval(const float min, const float max, const std::vector<Intervali> &intervals);

#endif //INTERVAL_HPP