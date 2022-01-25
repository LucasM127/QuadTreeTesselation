#include "QuadTreeTesselator.hpp"
#include "Quadrants.hpp"

namespace TESS
{

QuadTreeTesselator::QuadTreeTesselator(SZ width, SZ height, Vec2 offset, float stepSz)
    : m_width(width), m_height(height), m_offset(offset), m_stepSz(stepSz),
      m_quadTree(0,width,0,height)
{}

const std::vector<Point> &QuadTreeTesselator::getPoints() const
{
    return m_points;
}
const std::vector<PointId> &QuadTreeTesselator::getTriangles(const ID id) const
{
    if(m_polygons.find(id) == m_polygons.end())
        return m_nullVector;
    return m_polygons.at(id);
}

const std::vector<PointId> &QuadTreeTesselator::getCellTriIds(const ID id) const
{
    if(m_convexTriangleIds.find(id) == m_convexTriangleIds.end())
        return m_nullVector;
    return m_convexTriangleIds.at(id);
}

void QuadTreeTesselator::genGridPoints(const DQT::Node &node)
{
    if(!node.isLeaf)
    {//walk / diagonally from sw corner to ne corner
        genGridPoints(m_quadTree.sw(node));
        genGridPoints(m_quadTree.se(node));
        genGridPoints(m_quadTree.nw(node));
        genGridPoints(m_quadTree.ne(node));
        return;
    }

    const DQT::Node &northNode = m_quadTree.neighbour(node, DQT::DIR::NORTH);
    const DQT::Node &eastNode = m_quadTree.neighbour(node, DQT::DIR::EAST);
    const DQT::Node &southNode = m_quadTree.neighbour(node, DQT::DIR::SOUTH);
    const DQT::Node &westNode = m_quadTree.neighbour(node, DQT::DIR::WEST);

    CellInfo &C = m_cellInfos[node.id];
    const DQT::Box B = m_quadTree.calcBounds(node);

    //Gen SW Corner and share ?
    C.cornerIds[QUADRANT::SW] = m_points.size();
    if(westNode.id != DQT::INVALID_ID)
    {
        if(!westNode.isLeaf)//west is parent of smaller
        {
            const DQT::Node &wse = m_quadTree.se(westNode);
            m_cellInfos[wse.id].cornerIds[QUADRANT::SE] = m_points.size();
            m_cellInfos[node.id].steinerIds[DIR::LEFT] = m_cellInfos[wse.id].cornerIds[QUADRANT::NE];//se point to nothing of the box
        }
        else if(westNode.depth == node.depth)//same size
        {
            m_cellInfos[westNode.id].cornerIds[QUADRANT::SE] = m_points.size();
        }
        else//West is bigger
        {
            const DQT::Box w = m_quadTree.calcBounds(westNode);
            if(w.y == B.y)
                m_cellInfos[westNode.id].cornerIds[QUADRANT::SE] = m_points.size();
            else
                m_cellInfos[westNode.id].steinerIds[DIR::RIGHT] = m_points.size();
        }
    }
    if(southNode.id != DQT::INVALID_ID)
    {
        if(!southNode.isLeaf)
        {//and sw
            const DQT::Node &snw = m_quadTree.nw(southNode);
            m_cellInfos[snw.id].cornerIds[QUADRANT::NW] = m_points.size();
            m_cellInfos[node.id].steinerIds[DIR::DOWN] = m_cellInfos[snw.id].cornerIds[QUADRANT::NE];//nw point to nothing of the box
        }
        else if(southNode.depth == node.depth)
        {//and sw
            m_cellInfos[southNode.id].cornerIds[QUADRANT::NW] = m_points.size();
        }
        else//South is Bigger
        {//and sw but only if x==x
            const DQT::Box s = m_quadTree.calcBounds(southNode);
            if(s.x == B.x)
                m_cellInfos[southNode.id].cornerIds[QUADRANT::NW] = m_points.size();
            else
            {
                m_cellInfos[southNode.id].steinerIds[DIR::UP] = m_points.size();
                goto SOUTH_EXIT;
            }
        }//Now do sw
        if(westNode.id != DQT::INVALID_ID)
        {
            const DQT::Node &sw = m_quadTree.neighbour(southNode,DQT::DIR::WEST);
            if(sw.isLeaf)
                m_cellInfos[sw.id].cornerIds[QUADRANT::NE] = m_points.size();
            else
            {
                const DQT::Node &swne = m_quadTree.ne(sw);
                if(swne.isLeaf)
                    m_cellInfos[swne.id].cornerIds[QUADRANT::NE] = m_points.size();
                else
                {//balanced tree over two neighbour steps could be 4x smaller...  but not smaller
                    const DQT::Node &swnene = m_quadTree.ne(swne);
                    m_cellInfos[swnene.id].cornerIds[QUADRANT::NE] = m_points.size();
                }
            }
        }
    }
    SOUTH_EXIT:
    m_points.emplace_back(B.x,B.y);

    //If No Right Neighbour, Gen SE Corner
    if(eastNode.id == DQT::INVALID_ID)
    {
        C.cornerIds[QUADRANT::SE] = m_points.size();
        if(southNode.id != DQT::INVALID_ID)
        {
            if(southNode.isLeaf)
                m_cellInfos[southNode.id].cornerIds[QUADRANT::NE] = m_points.size();
            else
            {
                const DQT::Node &sne = m_quadTree.ne(southNode);
                m_cellInfos[sne.id].cornerIds[QUADRANT::NE] = m_points.size();
            }
        }
            
        m_points.emplace_back(B.x+B.sz,B.y);
    }
    else if(eastNode.depth < node.depth)
    {//Right neighbour is bigger, y's don't align... a local 'end' so no 'local' right neighbour
        const DQT::Box e = m_quadTree.calcBounds(eastNode);
        if(e.y != B.y)
        {
            C.cornerIds[QUADRANT::SE] = m_points.size();
            m_cellInfos[southNode.id].cornerIds[QUADRANT::NE] = m_points.size();
            m_points.emplace_back(B.x+B.sz,B.y);
        }
    }

    //If No North Neighbour, Gen NW Corner, same logic as No Right Neighbour
    if(northNode.id == DQT::INVALID_ID)
    {
        C.cornerIds[QUADRANT::NW] = m_points.size();
        if(westNode.id != DQT::INVALID_ID)
        {
            if(westNode.isLeaf)
                m_cellInfos[westNode.id].cornerIds[QUADRANT::NE] = m_points.size();
            else
            {
                const DQT::Node &wne = m_quadTree.ne(westNode);
                m_cellInfos[wne.id].cornerIds[QUADRANT::NE] = m_points.size();
            }
        }
        m_points.emplace_back(B.x,B.y+B.sz);
    }
    else if(northNode.depth < node.depth)
    {
        const DQT::Box n = m_quadTree.calcBounds(northNode);
        if(n.x != B.x)
        {
            C.cornerIds[QUADRANT::NW] = m_points.size();
            m_cellInfos[westNode.id].cornerIds[QUADRANT::NE] = m_points.size();
            m_points.emplace_back(B.x,B.y+B.sz);
        }
    }

    //If No NorthNeighbour And No East Neighbour Gen NE Corner (VERY END)
    if(northNode.id == DQT::INVALID_ID && eastNode.id == DQT::INVALID_ID)
    {
        C.cornerIds[QUADRANT::NE] = m_points.size();
        m_points.emplace_back(B.x+B.sz,B.y+B.sz);
        //FIN
    }
}
/*
    const DQT::Node &node = m_quadTree.node(0);
    DQT::Box n = m_quadTree.getNodeBounds(node);

    ID a = m_points.size();//nw
    m_points.emplace_back(n.x,n.y+n.size);
    ID b = m_points.size();//ne
    m_points.emplace_back(n.x+n.sz,n.y+n.size);
    ID c = m_points.size();//sw
    m_points.emplace_back(n.x,n.y);
    ID d = m_points.size();//se
    m_points.emplace_back(n.x+n.size,n.y);

    node->data.cornerIds[QUADRANT::NW] = a;
    node->data.cornerIds[QUADRANT::NE] = b;
    node->data.cornerIds[QUADRANT::SW] = c;
    node->data.cornerIds[QUADRANT::SE] = d;

    if(node->nw)
    {
        genGridPointsNW(node->nw);
        genGridPointsNE(node->ne);
        genGridPointsSW(node->sw);
        genGridPointsSE(node->se);
    }
}

void QuadTreeTesselator::genGridPointsNW(DQT::CellNode<CellInfo> *node)
{
    DQT::CellNode<CellInfo> *northNode = m_quadTree.at(node->x,node->y+node->size,node->depth);
    DQT::CellNode<CellInfo> *westNode  = m_quadTree.at(node->x-1,node->y,node->depth);

    ID a = node->parent->data.cornerIds[QUADRANT::NW];//nw
    
    ID b = m_points.size();//ne
    if(northNode)// && northNode->depth == node->depth) should be more like x + size - 1 for right one
        b = northNode->data.cornerIds[QUADRANT::SE];
    else
        m_points.emplace_back(node->x+node->size,node->y+node->size);
    
    ID c = m_points.size();//sw
    if(westNode)// && westNode->depth == node->depth)
        c = westNode->data.cornerIds[QUADRANT::SE];
    else
        m_points.emplace_back(node->x,node->y);
    
    ID d = m_points.size();//se
    m_points.emplace_back(node->x+node->size,node->y);

    node->data.cornerIds[QUADRANT::NW] = a;
    node->data.cornerIds[QUADRANT::NE] = b;
    node->data.cornerIds[QUADRANT::SW] = c;
    node->data.cornerIds[QUADRANT::SE] = d;

    if(node->nw)
    {
        genGridPointsNW(node->nw);
        genGridPointsNE(node->ne);
        genGridPointsSW(node->sw);
        genGridPointsSE(node->se);
    }
}

void QuadTreeTesselator::genGridPointsNE(DQT::CellNode<CellInfo> *node)
{
    ID a = node->parent->nw->data.cornerIds[QUADRANT::NE];//nw

    ID b = node->parent->data.cornerIds[QUADRANT::NE];//ne

    ID c = node->parent->nw->data.cornerIds[QUADRANT::SE];//sw

    ID d = m_points.size();//se
    m_points.emplace_back(node->x+node->size,node->y);

    node->data.cornerIds[QUADRANT::NW] = a;
    node->data.cornerIds[QUADRANT::NE] = b;
    node->data.cornerIds[QUADRANT::SW] = c;
    node->data.cornerIds[QUADRANT::SE] = d;

    if(node->nw)
    {
        genGridPointsNW(node->nw);
        genGridPointsNE(node->ne);
        genGridPointsSW(node->sw);
        genGridPointsSE(node->se);
    }
}

void QuadTreeTesselator::genGridPointsSW(DQT::CellNode<CellInfo> *node)
{
    ID a = node->parent->nw->data.cornerIds[QUADRANT::SW];//nw
    ID b = node->parent->nw->data.cornerIds[QUADRANT::SE];//ne
    ID c = node->parent->data.cornerIds[QUADRANT::SW];//sw
    ID d = m_points.size();//se
    m_points.emplace_back(node->x+node->size,node->y);
    node->data.cornerIds[QUADRANT::NW] = a;
    node->data.cornerIds[QUADRANT::NE] = b;
    node->data.cornerIds[QUADRANT::SW] = c;
    node->data.cornerIds[QUADRANT::SE] = d;

    if(node->nw)
    {
        genGridPointsNW(node->nw);
        genGridPointsNE(node->ne);
        genGridPointsSW(node->sw);
        genGridPointsSE(node->se);
    }
}

void QuadTreeTesselator::genGridPointsSE(DQT::CellNode<CellInfo> *node)
{
    ID a = node->parent->nw->data.cornerIds[QUADRANT::SE];//nw
    ID b = node->parent->ne->data.cornerIds[QUADRANT::SE];//ne
    ID c = node->parent->sw->data.cornerIds[QUADRANT::SE];//sw
    ID d = node->parent->data.cornerIds[QUADRANT::SE];//se

    node->data.cornerIds[QUADRANT::NW] = a;
    node->data.cornerIds[QUADRANT::NE] = b;
    node->data.cornerIds[QUADRANT::SW] = c;
    node->data.cornerIds[QUADRANT::SE] = d;

    if(node->nw)
    {
        genGridPointsNW(node->nw);
        genGridPointsNE(node->ne);
        genGridPointsSW(node->sw);
        genGridPointsSE(node->se);
    }
}*/

}//namespace TESS