#include "QuadTreeTesselator.hpp"
#include "Consts.hpp"
#include <cassert>
#include <cmath>

namespace TESS
{

static Point snap(const Point &p, const Vec2 &offset, const float stepSz)
{
    Point snappedPoint = p;
    snappedPoint = snappedPoint - offset;
    snappedPoint = snappedPoint / stepSz;
//    snappedPoint.x = roundf(snappedPoint.x);//Line clipped points won't work properly otherwise
//    snappedPoint.y = roundf(snappedPoint.y);
    return snappedPoint;
};

void QuadTreeTesselator::addLine(const std::vector<Point> &rline, ID rightId, ID leftId)
{
    std::vector<Point> line;
    line.reserve(rline.size());
    for(auto &p : rline)
        line.emplace_back(snap(p,m_offset,m_stepSz));
    m_lineDatas.push_back({std::move(line),rightId,leftId});
}

//assume line is 'IN Bounds' Clipping is out of this algorithms scope.
void QuadTreeTesselator::genLinePoints(QuadTreeTesselator::LineData &LD)
{
    PointId startId = INVALID_ID;//may or may not exist
    PointId lastId = INVALID_ID;
    PointId endId = INVALID_ID;

    bool isLooped = LD.line.front() == LD.line.back();
    size_t sz = LD.line.size();
    size_t i;

    for(i = 1; i < sz; ++i)//LD.line.size(); ++i)
    {
        Point a = LD.line[i-1];
        Point b = LD.line[i];
        if(a == b)
            continue;

        if(i == 1)
            castRay(a,b,LD.rightPolygonId,LD.leftPolygonId,startId,endId);//this is good (better have 2 points in the line... lol)
        else if(i == sz-1 && isLooped)
            castRay(a,b,LD.rightPolygonId,LD.leftPolygonId,endId,startId);
        else
        {
            lastId = endId;
            endId = INVALID_ID;
            castRay(a,b,LD.rightPolygonId,LD.leftPolygonId,lastId,endId);//not snapped here!
        }//if goes out of bounds endId is INVALID_ID
    }
    
    return;
}

static const Node * getRayNextNode(const Point &exitPoint, const QuadTree &quadTree, const bool headsRight, const bool headsUp, const Node * pnode);

//option: quit when distX == dx or distY == dy (same as we have... but with epsilon for if did with floaty point coords)
//with integer coords this seems to work fine as is no need to overly complicate things
void QuadTreeTesselator::castRay(const Point &a, const Point &b, ID rightPolygonId, ID leftPolygonId, PointId &startId, PointId &endId)
{
    //Has to start and end on a cell wall
    assert(a.x == roundf(a.x) || a.y == roundf(a.y));
    assert(b.x == roundf(b.x) || b.y == roundf(b.y));

    const float dx = b.x - a.x;
    const float dy = b.y - a.y;

    const float dDx = 1.f / fabsf(dx);
    const float dDy = 1.f / fabsf(dy);
    
    const float epsilon = 0.000001f;//~ 6 digits precision so ~100,000 max grid size i guess (?)

    float distX = 0.f;
    float distY = 0.f;

    bool headsRight = dx >= 0;
    bool headsUp    = dy >= 0;

    Point p(a);

    int loopCtr = 0;//if lastId is given use it for lastId
    PointId a_id, b_id;
    const Node *pnode = &m_quadTree.at(p.x,p.y,headsRight,headsUp);

    while (true)
    {   
        if(pnode->id == INVALID_ID)
        {
            endId = INVALID_ID;
            return;//the line went out of bounds of the gridMap
        }
        Box n = m_quadTree.calcBounds(*pnode);

        float min_x = n.x;
        float max_x = n.x + n.sz;
        float min_y = n.y;
        float max_y = n.y + n.sz;

        //for if line segment ends in the middle of a cell wall instead of on a corner nicely
        if(!headsRight && min_x < b.x)
            min_x = b.x;
        if(headsRight && max_x > b.x)
            max_x = b.x;
        if(!headsUp && min_y < b.y)
            min_y = b.y;
        if(headsUp && max_y > b.y)
            max_y = b.y;
        
        float x_dist_to_travel;
        float y_dist_to_travel;

        if(headsRight)
            x_dist_to_travel = n.x + n.sz - p.x;
        else
            x_dist_to_travel = p.x - n.x;
        if(headsUp)
            y_dist_to_travel = n.y + n.sz - p.y;
        else
            y_dist_to_travel = p.y - n.y;

        float stepX = x_dist_to_travel * dDx;
        float stepY = y_dist_to_travel * dDy;

        Point exitPoint;

        //Test if hit horizontal or vertical cell wall first and calculate intersection point
        if((stepX + epsilon) < stepY)
        {
            if(headsRight)
            {
                exitPoint.x = max_x;
                distX = max_x - a.x;//distX = (p.x - a.x) + x_dist_to_travel;
            }
            else
            {
                exitPoint.x = min_x;
                distX = min_x - a.x;//distX = (p.x - a.x) - x_dist_to_travel;
            }
            exitPoint.y = a.y + (dy * distX / dx);// m * distX;
        }
        else if((stepY + epsilon) < stepX)
        {
            if(headsUp)
            {
                exitPoint.y = max_y;
                distY = max_y - a.y;//(p.y - a.y) + y_dist_to_travel;
            }
            else
            {
                exitPoint.y = min_y;
                distY = min_y - a.y;//distY = (p.y - a.y) - y_dist_to_travel;
            }
            exitPoint.x = a.x + (dx * distY / dy);//a.x + m_ * distY;
        }
        else
        {
            if(headsRight)
            {
                exitPoint.x = max_x;
            }
            else
            {
                exitPoint.x = min_x;
            }
            if(headsUp)
            {
                exitPoint.y = max_y;
            }
            else
            {
                exitPoint.y = min_y;
            }
        }

//////END DDA RAY CAST STEP

        //Load the raycast generated points into the cellInfo struct
        CellInfo &C = m_cellInfos[pnode->id];
        if(C.lines.size() == 0)
            m_lineNodes.push_back(pnode);//add to active List for floodfill algo later

        //normalize the points
        Point a_normalized = p;
        Point b_normalized = exitPoint;
        a_normalized.x = (a_normalized.x - static_cast<float>(n.x)) / static_cast<float>(n.sz);
        a_normalized.y = (a_normalized.y - static_cast<float>(n.y)) / static_cast<float>(n.sz);
        b_normalized.x = (b_normalized.x - static_cast<float>(n.x)) / static_cast<float>(n.sz);
        b_normalized.y = (b_normalized.y - static_cast<float>(n.y)) / static_cast<float>(n.sz);

        //add normalized points to cellinfo
        C.lines.push_back(a_normalized);
        C.lines.push_back(b_normalized);
        C.linePolygonIds.push_back(rightPolygonId);
        C.linePolygonIds.push_back(leftPolygonId);

        bool isDone = exitPoint == b;//fix the exit condition

        if(loopCtr == 0) //starting the linesegment... are we starting it at any predefined grid points?
        {
            if(startId != INVALID_ID)
                a_id = startId;
            else if(a_normalized == Point(0,1))
                a_id = C.cornerIds[QUADRANT::NW];
            else if(a_normalized == Point(0,0))
                a_id = C.cornerIds[QUADRANT::SW];
            else if(a_normalized == Point(1,0))
                a_id = C.cornerIds[QUADRANT::SE];
            else if(a_normalized == Point(1,1))
                a_id = C.cornerIds[QUADRANT::NE];
            else if(a_normalized == Point(0.5f,1.0f) && C.steinerIds[DIR::NORTH] != INVALID_ID)
                a_id = C.steinerIds[DIR::NORTH];
            else if(a_normalized == Point(0.0f,0.5f) && C.steinerIds[DIR::WEST] != INVALID_ID)
                a_id = C.steinerIds[DIR::WEST];
            else if(a_normalized == Point(0.5f,0.0f) && C.steinerIds[DIR::SOUTH] != INVALID_ID)
                a_id = C.steinerIds[DIR::SOUTH];
            else if(a_normalized == Point(1.0f,0.5f) && C.steinerIds[DIR::EAST] != INVALID_ID)
                a_id = C.steinerIds[DIR::EAST];
            else //can remove duplicate line points later if it is an issue with removeDuplicateLinePoints() function
            {
                a_id = m_points.size();
                m_points.push_back(p);
            }
            startId = a_id;
        }
        else //continuing the linesegment raycast, just use the last id
            a_id = b_id;

        C.linePointIds.push_back(a_id);

        //See if b matches any preexisting points
        if(isDone && endId != INVALID_ID)//the polyline loops, ending, use the start point id I had saved
                b_id = endId;
        else if(b_normalized == Point(0,1))
            b_id = C.cornerIds[QUADRANT::NW];
        else if(b_normalized == Point(0,0))
            b_id = C.cornerIds[QUADRANT::SW];
        else if(b_normalized == Point(1,0))
            b_id = C.cornerIds[QUADRANT::SE];
        else if(b_normalized == Point(1,1))
            b_id = C.cornerIds[QUADRANT::NE];
        else if(b_normalized == Point(0.5f,1.0f) && C.steinerIds[DIR::NORTH] != INVALID_ID)
            b_id = C.steinerIds[DIR::NORTH];
        else if(b_normalized == Point(0.0f,0.5f) && C.steinerIds[DIR::WEST] != INVALID_ID)
            b_id = C.steinerIds[DIR::WEST];
        else if(b_normalized == Point(0.5f,0.0f) && C.steinerIds[DIR::SOUTH] != INVALID_ID)
            b_id = C.steinerIds[DIR::SOUTH];
        else if(b_normalized == Point(1.0f,0.5f) && C.steinerIds[DIR::EAST] != INVALID_ID)
            b_id = C.steinerIds[DIR::EAST];
        else
        {
            b_id = m_points.size();
            m_points.push_back(exitPoint);
        }
            
        C.linePointIds.push_back(b_id);

        if(isDone)
        {
            endId = b_id;
            return;
        }

        pnode = getRayNextNode(b_normalized, m_quadTree, headsRight, headsUp, pnode);
        assert(pnode->isLeaf);
        p = exitPoint;
        ++loopCtr;
    }
}

//Balanced tree assumed
static const Node * getRayNextNode(const Point &exitPoint, const QuadTree &quadTree, const bool headsRight, const bool headsUp, const Node * pnode)
{
    const int WEST = 1;
    const int EAST = 2;
    const int SOUTH = 4;
    const int NORTH = 8;

    int q = 0;//quadrant ray going into
    if(exitPoint.x == 0.f && !headsRight)
        q |= WEST;
    else if(exitPoint.x == 1.f)
        q |= EAST;
    if(exitPoint.y == 0.f && !headsUp)
        q |= SOUTH;
    else if(exitPoint.y == 1.f)
        q |= NORTH;

    //get appropiate neighbour node
    switch (q)
    {
    case NORTH:
        pnode = &quadTree.neighbour(*pnode, DIR::NORTH);
        break;
    case WEST:
        pnode = &quadTree.neighbour(*pnode, DIR::WEST);
        break;
    case SOUTH:
        pnode = &quadTree.neighbour(*pnode, DIR::SOUTH);
        break;
    case EAST:
        pnode = &quadTree.neighbour(*pnode, DIR::EAST);
        break;
    case SOUTH|WEST:
    {
        if(quadTree.neighbour(*pnode, DIR::WEST).depth < pnode->depth)
        {
            pnode = &quadTree.neighbour(*pnode, DIR::SOUTH);
            pnode = &quadTree.neighbour(*pnode, DIR::WEST);
        }
        else
        {
            pnode = &quadTree.neighbour(*pnode, DIR::WEST);
            pnode = &quadTree.neighbour(*pnode, DIR::SOUTH);
        }
    }
        break;
    case SOUTH|EAST:
    {
        if(quadTree.neighbour(*pnode, DIR::EAST).depth < pnode->depth)
        {
            pnode = &quadTree.neighbour(*pnode, DIR::SOUTH);
            pnode = &quadTree.neighbour(*pnode, DIR::EAST);
        }
        else
        {
            pnode = &quadTree.neighbour(*pnode, DIR::EAST);
            pnode = &quadTree.neighbour(*pnode, DIR::SOUTH);
        }
    }
        break;
    case NORTH|WEST:
    {
        if(quadTree.neighbour(*pnode, DIR::WEST).depth < pnode->depth)
        {
            pnode = &quadTree.neighbour(*pnode, DIR::NORTH);
            pnode = &quadTree.neighbour(*pnode, DIR::WEST);
        }
        else
        {
            pnode = &quadTree.neighbour(*pnode, DIR::WEST);
            pnode = &quadTree.neighbour(*pnode, DIR::NORTH);
        }
    }
        break;
    case NORTH|EAST:
    {
        if(quadTree.neighbour(*pnode, DIR::EAST).depth < pnode->depth)
        {
            pnode = &quadTree.neighbour(*pnode, DIR::NORTH);
            pnode = &quadTree.neighbour(*pnode, DIR::EAST);
        }
        else
        {
            pnode = &quadTree.neighbour(*pnode, DIR::EAST);
            pnode = &quadTree.neighbour(*pnode, DIR::NORTH);
        }
    }
        break;
    default:
        break;
    }

    if(pnode->id == INVALID_ID || pnode->isLeaf)
        return pnode;

    //Neighbour is a parent node, choose appropiate child

    switch (q)
    {
    case NORTH:
    {
        if(exitPoint.x < 0.5f)
            return &quadTree.sw(*pnode);
        else if(exitPoint.x > 0.5f)
            return &quadTree.se(*pnode);
        else
        {
            if(headsRight)
                return &quadTree.se(*pnode);
            else
                return &quadTree.sw(*pnode);
        }
    }
        break;
    case WEST:
    {
        if(exitPoint.y < 0.5f)
            return &quadTree.se(*pnode);
        else if(exitPoint.y > 0.5f)
            return &quadTree.ne(*pnode);
        else
        {
            if(headsUp)
                return &quadTree.ne(*pnode);
            else
                return &quadTree.se(*pnode);
        }
    }
        break;
    case SOUTH:
    {
        if(exitPoint.x < 0.5f)
            return &quadTree.nw(*pnode);
        else if(exitPoint.x > 0.5f)
            return &quadTree.ne(*pnode);//oops
        else
        {
            if(headsRight)
                return &quadTree.ne(*pnode);
            else
                return &quadTree.nw(*pnode);
        }
    }
        break;
    case EAST:
    {
        if(exitPoint.y < 0.5f)
            return &quadTree.sw(*pnode);
        else if(exitPoint.y > 0.5f)
            return &quadTree.nw(*pnode);
        else
        {
            if(headsUp)
                return &quadTree.nw(*pnode);
            else
                return &quadTree.sw(*pnode);
        }
    }
        break;
    case SOUTH|WEST:
    {
        pnode = &quadTree.ne(*pnode);
        if(!pnode->isLeaf)
            pnode = &quadTree.ne(*pnode);
        return pnode;
    }
        break;
    case SOUTH|EAST:
    {
        pnode = &quadTree.nw(*pnode);
        if(!pnode->isLeaf)
            pnode = &quadTree.nw(*pnode);
        return pnode;
    }
        break;
    case NORTH|WEST:
    {
        pnode = &quadTree.se(*pnode);
        if(!pnode->isLeaf)
            pnode = &quadTree.se(*pnode);
        return pnode;
    }
        break;
    case NORTH|EAST:
    {
        pnode = &quadTree.sw(*pnode);
        if(!pnode->isLeaf)
            pnode = &quadTree.sw(*pnode);
        return pnode;
    }
        break;
    default:
        break;
    }

    return pnode;
}

} //namespace TESS