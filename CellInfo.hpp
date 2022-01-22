#ifndef CELL_INFO_HPP
#define CELL_INFO_HPP

#include "Vec2.hpp"
#include <vector>
#include <cstdint>

namespace TESS
{

typedef uint32_t ID;
typedef uint32_t PointId;
typedef uint32_t SZ;

extern const ID INVALID_ID;
extern const ID EMPTY_SPACE_ID;

enum DIR : unsigned int
{
    UP = 0,
    LEFT,
    DOWN,
    RIGHT
};

struct CellInfo
{
    CellInfo() : steinerIds{INVALID_ID,INVALID_ID,INVALID_ID,INVALID_ID}{}
    PointId cornerIds[4];//abcd
    PointId steinerIds[4];

    std::vector<Point> lines;
    std::vector<PointId> linePointIds;
    std::vector<ID> linePolygonIds;

    std::vector<PointId> triangles;
    std::vector<ID> triangleIds;
    ID neighbourIds[4]; // for flood fill
    bool visited = false;
};

}//namespace TESS

#endif //CELL_INFO_HPP