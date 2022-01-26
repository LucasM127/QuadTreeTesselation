//TODOS left... figure out better ray cast step method
//Only other thing really that may be interesting to implement is
//adding floating point support.. but would have to create a dynamic tree structure for that
//cause otherwise it would be a point in a square, with uneven rectangular regions around it
//and (especially if is close to an edge), I do not think that would look nice in line with the rest of the map
//but maybe not???????
//But if you had an automatically updating mesh with dynamic geometry... that may look wonky

#ifndef QUAD_TREE_TESSELATOR_HPP
#define QUAD_TREE_TESSELATOR_HPP

#include "DQT.hpp"
#include "CellInfo.hpp"
#include <vector>

#include <unordered_map>

namespace TESS
{

class QuadTreeTesselator
{
public:
    QuadTreeTesselator(SZ width, SZ height, Vec2 offset, float stepSz);
    
    void addLine(const std::vector<Point> &line, ID rightId, ID leftId = TESS::EMPTY_SPACE_ID);
    void triangulate();

    const std::vector<Point> &getPoints() const;
    const std::vector<PointId> &getTriangles(const ID id) const;
    const std::vector<PointId> &getCellTriIds(const ID id) const;
private:

    struct LineData
    {
        const std::vector<Point> line;
        const ID rightPolygonId;
        const ID leftPolygonId;
    };

    const SZ m_width, m_height;
    const Vec2 m_offset;
    const float m_stepSz;
    SZ m_numGridPoints;
    DQT::QuadTree m_quadTree;
    std::vector<CellInfo> m_cellInfos;

    std::vector<PointId> m_nullVector;

    std::vector<Point> m_points;
    std::vector<const DQT::Node*> m_lineNodes;
    std::vector<LineData> m_lineDatas;
    std::unordered_map<ID,std::vector<PointId>> m_polygons;
    std::unordered_map<ID,std::vector<ID>> m_convexTriangleIds;//?

    void genGridPoints(const DQT::Node &node);

    void genLinePoints(LineData &LD);
    void removeDuplicateLinePoints();
    void fixCoords(float x, float y, bool headsRight, bool headsUp, int &px, int &py);
    void castRay(const Point &a, const Point &b, ID id, ID id_out, PointId &startId, PointId &endId);

    void floodFillTriangulate(const DQT::Node &node, ID polygonId, DIR skipDir);
};

} //namespace TESS

#endif //QUAD_TREE_TESSELATOR_HPP