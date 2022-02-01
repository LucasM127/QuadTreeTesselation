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
    //m_regions.insert(rightId);
    //m_regions.insert(leftId);
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

//Exit condition is SCARY but it hasn't failed me yet.  Fix one day.
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

    while (true)
    {
        const Node &node = m_quadTree.at(p.x,p.y,headsRight,headsUp);
        
        if(node.id == INVALID_ID)
        {
            endId = INVALID_ID;
            return;//the line went out of bounds of the gridMap
        }
        Box n = m_quadTree.calcBounds(node);

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
        CellInfo &C = m_cellInfos[node.id];
        if(C.lines.size() == 0)
            m_lineNodes.push_back(&node);//add to active List for floodfill algo later

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

        ++loopCtr;

        p = exitPoint;

        if(isDone)
        {
            endId = b_id;
            return;
        }

        p = exitPoint;
    }
}

} //namespace TESS