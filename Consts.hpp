#ifndef TESS_CONSTS_HPP
#define TESS_CONSTS_HPP

#include <cstdint>

namespace TESS
{
    
typedef uint32_t ID;
typedef uint32_t PointId;
typedef uint32_t NodeId;
typedef uint32_t SZ;

extern const ID INVALID_ID;
extern const ID EMPTY_SPACE_ID;

enum DIR : int
{
    NORTH = 0,
    EAST,
    SOUTH,
    WEST,
    NONE
};

enum QUADRANT : int
{
    SW = 0,
    SE,
    NW,
    NE
};

} //namespace TESS

#endif //TESS_CONSTS_HPP