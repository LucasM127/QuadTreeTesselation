#include "QuadTreeTesselator.hpp"
#include "TriangulateCellNode.hpp"

#include "Quadrants.hpp"
#include <chrono>
#include <iostream>

class Timer
{
public:
    Timer(const std::string &s) : string(s), start(std::chrono::high_resolution_clock::now()) {}
    ~Timer()
    {
        finish = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish-start);
        std::cout<<string<<" took "<<duration.count()<<" microseconds\n";
    }
    const std::string &string;
    std::chrono::time_point<std::chrono::high_resolution_clock> start, finish;
};

namespace TESS
{

void QuadTreeTesselator::triangulate()
{
    for(auto &LD : m_lineDatas)
    {
        for(auto &p : LD.line)
        {
            m_quadTree.subdivide(p.x,p.y);
        }
    }

    m_cellInfos.resize(m_quadTree.numLeaves());

{
    Timer T("Gen Grid Points");
    genGridPoints(m_quadTree.root());
    m_numGridPoints = m_points.size();
}
{
    Timer T("Gen Line Points");
    for(auto &LD : m_lineDatas)
    {
        genLinePoints(LD);
    }
}

    //Remove duplicate Points //Optional
    removeDuplicateLinePoints();
      
{
    Timer T("Triangulating1");//INDEED This is longer!
    for(const auto &node : m_lineNodes)
    {
        CellInfo &C = m_cellInfos[node->id];//m_quadTree.data(nodeId);

        triangulateCellNode(C);
        C.visited = true;

        //output the triangle data... to the polygons...
        for(size_t i = 0; i < C.triangleIds.size(); ++i)
        {
            ID triangleId = C.triangleIds[i];
            PointId a = C.triangles[3*i+0];
            PointId b = C.triangles[3*i+1];
            PointId c = C.triangles[3*i+2];
            
            m_polygons[triangleId].push_back(a);
            m_polygons[triangleId].push_back(b);
            m_polygons[triangleId].push_back(c);
            //each cellNode has a unique id... use the id of the cellNode 
            m_convexTriangleIds[triangleId].push_back(node->id);
        }
    }

    //and then revert to world positions again
    for(auto &p : m_points)
    {
        p.x *= m_stepSz;
        p.y *= m_stepSz;
        p = p + m_offset;
    }
}
{
    Timer T("FloodFill");
    if(m_lineNodes.size() == 0)
        floodFillTriangulate(m_quadTree.at(0,0),TESS::EMPTY_SPACE_ID,DIR::LEFT);
    for(auto node : m_lineNodes)
    {
        const CellInfo &C = m_cellInfos[node->id];
        
        const DQT::Node &northNode = m_quadTree.neighbour(*node,DQT::DIR::NORTH);
        const DQT::Node &southNode = m_quadTree.neighbour(*node,DQT::DIR::SOUTH);
        const DQT::Node &eastNode = m_quadTree.neighbour(*node,DQT::DIR::EAST);
        const DQT::Node &westNode = m_quadTree.neighbour(*node,DQT::DIR::WEST);       

        if(C.neighbourIds[DIR::UP] != INVALID_ID && northNode.id != DQT::INVALID_ID)
        {
            if(northNode.isLeaf)
                floodFillTriangulate(northNode,C.neighbourIds[DIR::UP],DIR::DOWN);
            else
                floodFillTriangulate(m_quadTree.sw(northNode), C.neighbourIds[DIR::UP],DIR::DOWN);
        }
        if(C.neighbourIds[DIR::DOWN] != INVALID_ID && southNode.id != DQT::INVALID_ID)
        {
            if(southNode.isLeaf)
                floodFillTriangulate(southNode,C.neighbourIds[DIR::DOWN],DIR::UP);
            else
                floodFillTriangulate(m_quadTree.nw(southNode), C.neighbourIds[DIR::DOWN],DIR::UP);
        }
        if(C.neighbourIds[DIR::LEFT] != INVALID_ID && westNode.id != DQT::INVALID_ID)
        {
            if(westNode.isLeaf)
                floodFillTriangulate(westNode,C.neighbourIds[DIR::LEFT],DIR::RIGHT);
            else
                floodFillTriangulate(m_quadTree.se(westNode), C.neighbourIds[DIR::LEFT],DIR::RIGHT);
        }
        if(C.neighbourIds[DIR::RIGHT] != INVALID_ID && eastNode.id != DQT::INVALID_ID)
        {
            if(eastNode.isLeaf)
                floodFillTriangulate(eastNode,C.neighbourIds[DIR::RIGHT],DIR::LEFT);
            else
                floodFillTriangulate(m_quadTree.sw(eastNode), C.neighbourIds[DIR::RIGHT],DIR::LEFT);
        }

    }
/*
   for(size_t i = 0; i < m_cellInfos.size(); ++i)
   {
       std::cout<<i<<": "<<m_cellInfos[i].cornerIds[0]<<" "<<m_cellInfos[i].cornerIds[1]<<" "<<m_cellInfos[i].cornerIds[2]<<" "<<m_cellInfos[i].cornerIds[3]<<"\n";
   }
*/
}
}

//oops add polygon Id but later...
void QuadTreeTesselator::floodFillTriangulate(const DQT::Node &node, ID polygonId, DIR skipDir)
{
///
    if(node.id == DQT::INVALID_ID)
        return;

    CellInfo &C = m_cellInfos[node.id];//m_quadTree.data(nodeId);

    if(C.visited)
        return;

    C.visited = true;

    //create the triangles
    PointId ne = C.cornerIds[QUADRANT::NE];
    PointId nw = C.cornerIds[QUADRANT::NW];
    PointId se = C.cornerIds[QUADRANT::SE];
    PointId sw = C.cornerIds[QUADRANT::SW];

    PointId up = C.steinerIds[DIR::UP];
    PointId right = C.steinerIds[DIR::RIGHT];
    PointId down = C.steinerIds[DIR::DOWN];
    PointId left = C.steinerIds[DIR::LEFT];

    int k = 0;
    if(up != INVALID_ID) k += 1;
    if(right != INVALID_ID) k += 2;
    if(down != INVALID_ID) k += 4;
    if(left != INVALID_ID) k += 8;

    auto addTriangle = [&](PointId a, PointId b, PointId c)
    {
        m_polygons[polygonId].push_back(a);
        m_polygons[polygonId].push_back(b);
        m_polygons[polygonId].push_back(c);
        m_convexTriangleIds[polygonId].push_back(node.id);
    };

//#ifndef INCLUDE_STEINER_POINTS
//    k = 0;
//#endif

    switch (k)
    {
    case 0:
        addTriangle(ne,sw,nw);
        addTriangle(sw,ne,se);
        break;
    case 1:
        addTriangle(sw,nw,up);
        addTriangle(sw,up,se);
        addTriangle(se,up,ne);
        break;
    case 2:
        addTriangle(nw,ne,right);
        addTriangle(nw,right,sw);
        addTriangle(sw,right,se);
        break;
    case 3:
        addTriangle(sw,nw,up);
        addTriangle(sw,up,right);
        addTriangle(sw,right,se);
        addTriangle(right,up,ne);
        break;
    case 4:
        addTriangle(sw,nw,down);
        addTriangle(nw,ne,down);
        addTriangle(ne,se,down);
        break;
    case 5:
        addTriangle(sw,nw,up);
        addTriangle(sw,up,down);
        addTriangle(down,up,ne);
        addTriangle(down,ne,se);
        break;
    case 6:
        addTriangle(sw,nw,down);
        addTriangle(nw,right,down);
        addTriangle(nw,ne,right);
        addTriangle(down,right,se);
        break;
    case 7:
        addTriangle(sw,nw,down);
        addTriangle(nw,up,down);
        addTriangle(down,up,right);
        addTriangle(right,up,ne);
        addTriangle(down,right,se);
        break;
    case 8:
        addTriangle(sw,left,se);
        addTriangle(left,ne,se);
        addTriangle(left,nw,ne);
        break;
    case 9:
        addTriangle(sw,left,se);
        addTriangle(se,left,up);
        addTriangle(se,up,ne);
        addTriangle(left,nw,up);
        break;
    case 10:
        addTriangle(sw,right,se);
        addTriangle(sw,left,right);
        addTriangle(left,nw,right);
        addTriangle(nw,ne,right);
        break;
    case 11:
        addTriangle(sw,right,se);
        addTriangle(sw,left,right);
        addTriangle(left,up,right);
        addTriangle(left,nw,up);
        addTriangle(right,up,ne);
        break;
    case 12:
        addTriangle(ne,left,nw);
        addTriangle(ne,down,left);
        addTriangle(ne,se,down);
        addTriangle(down,sw,left);
        break;
    case 13:
        addTriangle(sw,left,down);
        addTriangle(left,up,down);
        addTriangle(left,nw,up);
        addTriangle(down,up,ne);
        addTriangle(down,ne,se);
        break;
    case 14:
        addTriangle(sw,left,down);
        addTriangle(down,right,se);
        addTriangle(left,right,down);
        addTriangle(left,ne,right);
        addTriangle(left,nw,ne);
        break;
    case 15:
        addTriangle(sw,left,down);
        addTriangle(nw,up,left);
        addTriangle(ne,right,up);
        addTriangle(se,down,right);
        addTriangle(down,left,up);
        addTriangle(down,up,right);
        break;
    default:
        break;
    }

    const DQT::Node &northNode = m_quadTree.neighbour(node,DQT::DIR::NORTH);
    const DQT::Node &southNode = m_quadTree.neighbour(node,DQT::DIR::SOUTH);
    const DQT::Node &eastNode = m_quadTree.neighbour(node,DQT::DIR::EAST);
    const DQT::Node &westNode = m_quadTree.neighbour(node,DQT::DIR::WEST);

    if(northNode.id != DQT::INVALID_ID && skipDir != DIR::UP)
    {
        if(northNode.isLeaf)
            floodFillTriangulate(northNode,polygonId,DIR::DOWN);
        else
            floodFillTriangulate(m_quadTree.sw(northNode), polygonId,DIR::DOWN);
    }
    if(southNode.id != DQT::INVALID_ID && skipDir != DIR::DOWN)
    {
        if(southNode.isLeaf)
            floodFillTriangulate(southNode,polygonId,DIR::UP);
        else
            floodFillTriangulate(m_quadTree.nw(southNode), polygonId,DIR::UP);
    }
    if(westNode.id != DQT::INVALID_ID && skipDir != DIR::LEFT)
    {
        if(westNode.isLeaf)
            floodFillTriangulate(westNode,polygonId,DIR::RIGHT);
        else
            floodFillTriangulate(m_quadTree.se(westNode), polygonId,DIR::RIGHT);
    }
    if(eastNode.id != DQT::INVALID_ID && skipDir != DIR::RIGHT)
    {
        if(eastNode.isLeaf)
            floodFillTriangulate(eastNode,polygonId,DIR::LEFT);
        else
            floodFillTriangulate(m_quadTree.sw(eastNode), polygonId,DIR::LEFT);
    }
}

} //namespace TESS

/* Doesn't look as nice
    PointId mid = m_points.size();
    m_points.emplace_back((m_points[sw]+m_points[ne]) / 2.f);

    if(up != INVALID_ID)
    {
        addTriangle(nw,up,mid);
        addTriangle(up,ne,mid);
    }
    else
        addTriangle(nw,ne,mid);
    if(right != INVALID_ID)
    {
        addTriangle(ne,right,mid);
        addTriangle(right,se,mid);
    }
    else
        addTriangle(ne,se,mid);
    if(down != INVALID_ID)
    {
        addTriangle(se,down,mid);
        addTriangle(down,sw,mid);
    }
    else
        addTriangle(se,sw,mid);
    if(left != INVALID_ID)
    {
        addTriangle(sw,left,mid);
        addTriangle(left,nw,mid);
    }
    else
        addTriangle(sw,nw,mid);
*/