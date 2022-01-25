#ifndef INTERVAL_HPP
#define INTERVAL_HPP

#include <cassert>
#include <vector>

//SUCKS
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
    inline bool contains(float min, float max) const
    {
        return max <= this->max && min >= this->min;
    }
    inline bool contains(const float f) const
    {
        return f >= min && f <= max;
    }
    inline bool operator==(const Intervali &I)
    {
        return I.min == min && I.max == max && I.val == val;
    }
    float min, max;
    unsigned int val;
};

void insert(Intervali &I, std::vector<Intervali> &intervals);

std::vector<Intervali> queryInterval(const float min, const float max, const std::vector<Intervali> &intervals);

#endif //INTERVAL_HPP