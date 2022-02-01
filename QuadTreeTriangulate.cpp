#include "QuadTreeTesselator.hpp"
#include "TriangulateCellNode.hpp"

#include "Consts.hpp"

namespace TESS
{

//static const SZ NUM_CELL_POINTS_PER_REGION = ###;
//static const SZ NUM_POINTS = ###;
//static const SZ NUM_TRI_POINTS_PER_REGION = ###;

void QuadTreeTesselator::triangulate()
{
    for(auto &LD : m_lineDatas)
    {
        for(auto &p : LD.line)
        {
            m_quadTree.subdivide(p.x,p.y);
        }
    }
/*
    //Doesn't really make much difference but a slight difference (speed wise) not worth it?
    for(auto r : m_regions)
    {
        m_convexTriangleIds[r].reserve(NUM_CELL_POINTS_PER_REGION);
        m_polygons[r].reserve(NUM_TRI_POINTS_PER_REGION);
    }
    m_points.reserve(NUM_POINTS);
*/
    m_cellInfos.resize(m_quadTree.numLeaves());

    genGridPoints(m_quadTree.root());
    m_numGridPoints = m_points.size();

    for(auto &LD : m_lineDatas)
    {
        genLinePoints(LD);
    }

    //Remove duplicate Points //Optional
    removeDuplicateLinePoints();
      
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

    if(m_lineNodes.size() == 0)
        floodFillTriangulate(m_quadTree.at(0,0),TESS::EMPTY_SPACE_ID,DIR::WEST);
    for(auto node : m_lineNodes)
    {
        const CellInfo &C = m_cellInfos[node->id];
        
        const Node &northNode = m_quadTree.neighbour(*node,DIR::NORTH);
        const Node &southNode = m_quadTree.neighbour(*node,DIR::SOUTH);
        const Node &eastNode  = m_quadTree.neighbour(*node,DIR::EAST);
        const Node &westNode  = m_quadTree.neighbour(*node,DIR::WEST);       

        if(C.neighbourIds[DIR::NORTH] != INVALID_ID && northNode.id != INVALID_ID)
        {
            if(northNode.isLeaf)
                floodFillTriangulate(northNode,C.neighbourIds[DIR::NORTH],DIR::SOUTH);
            else
                floodFillTriangulate(m_quadTree.sw(northNode), C.neighbourIds[DIR::NORTH],DIR::SOUTH);
        }
        if(C.neighbourIds[DIR::SOUTH] != INVALID_ID && southNode.id != INVALID_ID)
        {
            if(southNode.isLeaf)
                floodFillTriangulate(southNode,C.neighbourIds[DIR::SOUTH],DIR::NORTH);
            else
                floodFillTriangulate(m_quadTree.nw(southNode), C.neighbourIds[DIR::SOUTH],DIR::NORTH);
        }
        if(C.neighbourIds[DIR::WEST] != INVALID_ID && westNode.id != INVALID_ID)
        {
            if(westNode.isLeaf)
                floodFillTriangulate(westNode,C.neighbourIds[DIR::WEST],DIR::EAST);
            else
                floodFillTriangulate(m_quadTree.se(westNode), C.neighbourIds[DIR::WEST],DIR::EAST);
        }
        if(C.neighbourIds[DIR::EAST] != INVALID_ID && eastNode.id != INVALID_ID)
        {
            if(eastNode.isLeaf)
                floodFillTriangulate(eastNode,C.neighbourIds[DIR::EAST],DIR::WEST);
            else
                floodFillTriangulate(m_quadTree.sw(eastNode), C.neighbourIds[DIR::EAST],DIR::WEST);
        }

    }
}

void QuadTreeTesselator::floodFillTriangulate(const Node &node, ID polygonId, DIR skipDir)
{
    if(node.id == INVALID_ID)
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

    PointId up = C.steinerIds[DIR::NORTH];
    PointId right = C.steinerIds[DIR::EAST];
    PointId down = C.steinerIds[DIR::SOUTH];
    PointId left = C.steinerIds[DIR::WEST];

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

    const Node &northNode = m_quadTree.neighbour(node,DIR::NORTH);
    const Node &southNode = m_quadTree.neighbour(node,DIR::SOUTH);
    const Node &eastNode  = m_quadTree.neighbour(node,DIR::EAST);
    const Node &westNode  = m_quadTree.neighbour(node,DIR::WEST);

    if(northNode.id != INVALID_ID && skipDir != DIR::NORTH)
    {
        if(northNode.isLeaf)
            floodFillTriangulate(northNode,polygonId,DIR::SOUTH);
        else
            floodFillTriangulate(m_quadTree.sw(northNode), polygonId,DIR::SOUTH);
    }
    if(southNode.id != INVALID_ID && skipDir != DIR::SOUTH)
    {
        if(southNode.isLeaf)
            floodFillTriangulate(southNode,polygonId,DIR::NORTH);
        else
            floodFillTriangulate(m_quadTree.nw(southNode), polygonId,DIR::NORTH);
    }
    if(westNode.id != INVALID_ID && skipDir != DIR::WEST)
    {
        if(westNode.isLeaf)
            floodFillTriangulate(westNode,polygonId,DIR::EAST);
        else
            floodFillTriangulate(m_quadTree.se(westNode), polygonId,DIR::EAST);
    }
    if(eastNode.id != INVALID_ID && skipDir != DIR::EAST)
    {
        if(eastNode.isLeaf)
            floodFillTriangulate(eastNode,polygonId,DIR::WEST);
        else
            floodFillTriangulate(m_quadTree.sw(eastNode), polygonId,DIR::WEST);
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