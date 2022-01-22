#include "QuadTreeTesselator.hpp"
#include "Quadrants.hpp"
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

//TODO: Remove the requirement to use epsilon
void QuadTreeTesselator::castRay(const Point &a, const Point &b, ID rightPolygonId, ID leftPolygonId, PointId &startId, PointId &endId)
{
    //Has to start and end on a cell wall
    assert(a.x == roundf(a.x) || a.y == roundf(a.y));
    assert(b.x == roundf(b.x) || b.y == roundf(b.y));

    const float dx = b.x - a.x;
    const float dy = b.y - a.y;

    const float dDx = 1.f / fabsf(dx);
    const float dDy = 1.f / fabsf(dy);

    const float m  = dy/dx;
    const float m_ = dx/dy;
//play with epsilon... (?) //REMOVE
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
        const DQT::Node &node = m_quadTree.at(p.x,p.y,headsRight,headsUp);
        
        if(node.id == DQT::INVALID_ID)
        {
            endId = INVALID_ID;
            return;//the line went out of bounds of the gridMap
        }
        DQT::Box n = m_quadTree.calcBounds(node);

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
            exitPoint.y = a.y + m * distX;
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
            exitPoint.x = a.x + m_ * distY;
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
//#ifdef INCLUDE_STEINER_POINTS
            else if(a_normalized == Point(0.5f,1.0f) && C.steinerIds[DIR::UP] != INVALID_ID)
                a_id = C.steinerIds[DIR::UP];
            else if(a_normalized == Point(0.0f,0.5f) && C.steinerIds[DIR::LEFT] != INVALID_ID)
                a_id = C.steinerIds[DIR::LEFT];
            else if(a_normalized == Point(0.5f,0.0f) && C.steinerIds[DIR::DOWN] != INVALID_ID)
                a_id = C.steinerIds[DIR::DOWN];
            else if(a_normalized == Point(1.0f,0.5f) && C.steinerIds[DIR::RIGHT] != INVALID_ID)
                a_id = C.steinerIds[DIR::RIGHT];
//#endif
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
        else if(b_normalized == Point(0.5f,1.0f) && C.steinerIds[DIR::UP] != INVALID_ID)
            b_id = C.steinerIds[DIR::UP];
        else if(b_normalized == Point(0.0f,0.5f) && C.steinerIds[DIR::LEFT] != INVALID_ID)
            b_id = C.steinerIds[DIR::LEFT];
        else if(b_normalized == Point(0.5f,0.0f) && C.steinerIds[DIR::DOWN] != INVALID_ID)
            b_id = C.steinerIds[DIR::DOWN];
        else if(b_normalized == Point(1.0f,0.5f) && C.steinerIds[DIR::RIGHT] != INVALID_ID)
            b_id = C.steinerIds[DIR::RIGHT];
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


/*
void QuadTreeTesselator::addLine(const std::vector<Point> &rline, ID id, bool isLooped)
{
//bit verbose though but works :)
    if(!rline.size())
        return;
    
    std::vector<Point> *v;
    bool startNewLine = true;
    size_t l = m_lineDatas.size();
    for(size_t i = 1; i < rline.size(); ++i)
    {
        const Point a = snap(rline[i-1],m_offset,m_stepSz);
        const Point b = snap(rline[i],m_offset,m_stepSz);
        Point entryPt, exitPt;

        int clipType = clipLine(a,b,m_width,m_height,entryPt,exitPt);
        if(clipType == CLIP_IN)//??? WHY NOT
        {
            if(startNewLine)
            {
                m_lineDatas.push_back({std::vector<Point>(), id});
                m_lineDatas.back().line.push_back(entryPt);
                startNewLine = false;
            }
            m_lineDatas.back().line.push_back(exitPt);
        }
        else if(clipType == CLIP_A)//starts new line
        {
            m_lineDatas.push_back({std::vector<Point>(), id});
            m_lineDatas.back().line.push_back(entryPt);
            startNewLine = false;
            m_lineDatas.back().line.push_back(exitPt);
        }
        else if(clipType == CLIP_B)
        {
            if(startNewLine)
            {
                m_lineDatas.push_back({std::vector<Point>(), id});
                m_lineDatas.back().line.push_back(entryPt);
                //startNewLine = false;
            }
            m_lineDatas.back().line.push_back(exitPt);
            startNewLine = true;
        }
        else if(clipType == CLIP_BOTH)
        {
            m_lineDatas.push_back({std::vector<Point>(), id});
            m_lineDatas.back().line.push_back(entryPt);
            startNewLine = false;
            m_lineDatas.back().line.push_back(exitPt);
        }
        else if (clipType == CLIP_OUT)
            ;
    }//ok
    
    if(l + 1 < m_lineDatas.size())
    {
        if(m_lineDatas[l].line[0] == m_lineDatas.back().line.back())//loopy case
        {
            m_lineDatas.back().line.insert(m_lineDatas.back().line.end(),std::next(m_lineDatas[l].line.begin()),m_lineDatas[l].line.end());
            m_lineDatas[l].line.clear();//erase it there ????
        }
    }

//Then just add a check... see if looped
//then see if first point and last point are still the same?
//then just append first to last if clipping had occurred.
    
// / *
    float min_x = m_offset.x;
    float min_y = m_offset.y;
    float max_x = min_x + static_cast<float>(m_width) * m_stepSz;
    float max_y = min_y + static_cast<float>(m_height) * m_stepSz;
    bool isContained = true;
    for(auto &p : line)
    {
        if(p.x < min_x || p.x > max_x || p.y < min_y || p.y > max_y)
        {
            isContained = false;
            break;
        }
    }// * /
//    bool isContained = true;
//    m_lineDatas.push_back({line,id,isLooped,isContained});
}
*/

/*
static const int CLIP_IN = 0;
static const int CLIP_A = 1;
static const int CLIP_B = 2;
static const int CLIP_BOTH = 3;
static const int CLIP_OUT = 4;

//modified from https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
//(0-w)X(0-h) box
static int clipLine(const Point &a, const Point &b, const float w, const float h, Point &entryPt, Point &exitPt)
{
    typedef int OutCode;
    const int INSIDE = 0;
    const int LEFT = 1;
    const int RIGHT = 2;
    const int BOTTOM = 4;
    const int TOP = 8;

    auto computeOutCode = [](const Point &p, const float w, const float h)->OutCode
    {
        OutCode code = INSIDE;

        if(p.x < 0)
            code |= LEFT;
        else if(p.x > w)
            code |= RIGHT;
        if(p.y < 0)
            code |= BOTTOM;
        else if(p.y > h)
            code |= TOP;

        return code;
    };

    entryPt = a;
    exitPt = b;
    
    OutCode outCodeA = computeOutCode(entryPt,w,h);
    OutCode outCodeB = computeOutCode(exitPt,w,h);

    int clipType = CLIP_IN;
    
    while (true)
    {
        if(!(outCodeA | outCodeB))
            return clipType;//both points inside !(00|00)
        if (outCodeA & outCodeB)
            return CLIP_OUT;//both points share exclusion zone... so no way can go in
        
        OutCode theOutsideOutcode = outCodeA > outCodeB ? outCodeA : outCodeB;//so if one is inside (00) choose the outside outcode

        float x,y;
        //find where clips rectangle lines .. clip to line... repeats loop if and only if both endpoints lay outside rectangle
        if(theOutsideOutcode & TOP)
        {
            x = a.x + (b.x - a.x) * (h - a.y) / (b.y - a.y);
            y = h;
        }
        else if(theOutsideOutcode & BOTTOM)
        {
            x = a.x + (b.x - a.x) * (0 - a.y) / (b.y - a.y);
            y = 0;
        }
        else if(theOutsideOutcode & RIGHT)
        {
            y = a.y + (b.y - a.y) * (w - a.x) / (b.x - a.x);//w
            x = w;
        }
        else// if(theOutsideOutcode & LEFT)
        {
            y = a.y + (b.y - a.y) * (0 - a.x) / (b.x - a.x);
            x = 0;
        }

        if(theOutsideOutcode == outCodeA)
        {
            clipType |= CLIP_A;
            entryPt.x = x;
            entryPt.y = y;
            outCodeA = computeOutCode(entryPt,w,h);
        }
        else
        {
            clipType |= CLIP_B;
            exitPt.x = x;
            exitPt.y = y;
            outCodeB = computeOutCode(exitPt,w,h);
        }
    }   
}
*/

/*
static bool lineIntersectsYAtX(const Point &a, const Point &b, float x)
{
    return (a.x > x && b.x < x) || (b.x > x && a.x < x);
}

static bool lineIntersectsXAtY(const Point &a, const Point &b, float y)
{
    return (a.y > y && b.y < y) || (b.y > y && a.y < y);
}

static Point intYatXPoint(const Point &a, const Point &b, float x)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float m = dy/dx;
    float y = a.y + m * (x-a.x);
    return Point(x,y);
}

static Point intXatYPoint(const Point &a, const Point &b, float y)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float m_ = dx/dy;
    float x = a.x + m_ * (y-a.y);
    return Point(x,y);
}
*/

/* Line clipping... Doesn't work ... redo
    Point startPoint(a);

    //modified from https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm for the quadrantCode idea kinda should use this more...
    {
    float width = static_cast<float>(m_width);
    float height = static_cast<float>(m_height);

    const int INSIDE = 0;
    const int LEFT = 1;
    const int RIGHT = 2;
    const int BOTTOM = 4;
    const int TOP = 8;

    int outCode = INSIDE;

    if(a.x < 0.f)
        outCode |= LEFT;
    else if(a.x > width)
        outCode |= RIGHT;
    if(a.y < 0.f)
        outCode |= BOTTOM;
    else if(a.y > height)
        outCode |= TOP;

    if(outCode != INSIDE)
    {
        startId = INVALID_ID;

        if(headsRight && outCode == RIGHT)
            return false;
        if(!headsRight && outCode == LEFT)
            return false;
        if(headsUp && outCode == TOP)
            return false;
        if(!headsUp && outCode == BOTTOM)
            return false;

        int outCodeB = INSIDE;
        if(a.x < 0.f)
            outCodeB |= LEFT;
        else if(a.x > width)
            outCodeB |= RIGHT;
        if(a.y < 0.f)
            outCodeB |= BOTTOM;
        else if(a.y > height)
            outCodeB |= TOP;
        if(outCode & LEFT && outCodeB & LEFT)
            return false;
        if(outCode & RIGHT && outCodeB & RIGHT)
            return false;
        if(outCode & TOP && outCodeB & TOP)
            return false;
        if(outCode & BOTTOM && outCodeB & BOTTOM)
            return false;

        //get x int at y or y int at x (of closer wall) and see if is in bounds!
        float X = 0.f;
        if(!headsRight)
            X = width;
        float y = a.y + m * (X - a.x);
        bool yInBounds = (y >= 0.f && y <= height);

        float Y = 0.f;
        if(!headsUp)
            Y = height;
        float x = a.x + m_ * (Y - a.y);
        bool xInBounds = (x >= 0.f && x <= width);

        if(outCode == LEFT || outCode == RIGHT)
        {
            if(!yInBounds)
                return false;
            startPoint.x = X;
            startPoint.y = y;
        }
        else if(outCode == TOP || outCode == BOTTOM)
        {
            if(!xInBounds)
                return false;
            startPoint.x = x;
            startPoint.y = Y;
        }
        else//in one of the quadrants...
        {
            if(xInBounds)
            {
                startPoint.x = x;
                startPoint.y = Y;
            }
            else if(yInBounds)
            {
                startPoint.x = X;
                startPoint.y = y;
            }
            else
                return false;
        }
    }
    }
//untested
    if(startPoint == b)
        return false;
*/

/*
    bool startNewLine = true;
    float max_x = static_cast<float>(m_width);
    float max_y = static_cast<float>(m_height);

    for(size_t i = 1; i < LD.line.size(); ++i)
    {
        Point a = snap(LD.line[i-1],m_offset,m_stepSz);
        Point b = snap(LD.line[i  ],m_offset,m_stepSz);

        //if fullycontained works great...
        if(a.x <= max_x && a.y <= max_y &&
           b.x <= max_x && b.y <= max_y)
        {
            //startId is id made at the beginning of the line... may or may not exist!
            //HELP ME THIS LOGIC SUCKS
            if(startNewLine)//start new line
            {
                castRay(a,b,LD.id,lastId,endId);
                if(i == 1) startId = lastId;
                startNewLine = false;
            }
            else
            {
                lastId = endId;
                endId = INVALID_ID;
                castRay(m_points[lastId],b,LD.id,lastId,endId);
            }
        }
        //else just screw it....
        
        else if(lineIntersectsYAtX(a,b,max_x))
        {//
            Point p = intYatXPoint(a,b,max_x);
            if(a.x <= max_x && a.y <= max_y)//line exits grid here
            {
                if(startNewLine)
                {//more helperish functions than actual function calls? lambdas?
                    castRay(a,p,LD.id,lastId,endId);
                    if(i == 1) startId = lastId;
                }
                else//continueLine(p,LD.id)
                {
                    lastId = endId;
                    endId = INVALID_ID;
                    castRay(m_points[lastId],p,LD.id,lastId,endId);
                }
                startNewLine = true;
                lastId = endId = INVALID_ID;
            }
            else if(b.x <= max_x && b.y <= max_y)//line starts here
            {
                assert(startNewLine == true);
                castRay(p,b,LD.id,lastId,endId);
                startNewLine = false;
            }
        }
        else if(lineIntersectsXAtY(a,b,max_y))
        {//
            Point p = intXatYPoint(a,b,max_y);
            if(a.x <= max_x && a.y <= max_y)//line exits grid here
            {
                if(startNewLine)
                {//more helperish functions than actual function calls? lambdas?
                    castRay(a,p,LD.id,lastId,endId);
                    if(i == 1) startId = lastId;
                }
                else//continueLine(p,LD.id)
                {
                    lastId = endId;
                    endId = INVALID_ID;
                    castRay(m_points[lastId],p,LD.id,lastId,endId);
                }
                startNewLine = true;
                lastId = endId = INVALID_ID;
            }
            else if(b.x <= max_x && b.y <= max_y)//line starts here
            {
                assert(startNewLine == true);
                castRay(p,b,LD.id,lastId,endId);
                startNewLine = false;
            }
        }
    }
    if(LD.isLooped)
    {
        //try cast a ray from endId -> startId if startId is valid
        //if startId is invalid -> the end point is out of bounds clip it
        //if startNewLine = true the start point is out of bounds
        //if both are out of bounds... they may still intersect the grid though ... sigh
        //point is first point and last point
        //castRayassumes is within bounds.... give it that way so can't check lastId as that would still be valid...
        //but startNewLine -> invalidates it.
    }

    //sizex size y... m_width = float m_stepWidth is int
*/
/*
    if(LD.isContained)//Had to test each vertice and see if is in bounds for this... :/ kinda redundant
    {
        castRay(LD.line[0],LD.line[1],LD.id,startId,endId);
        for(size_t i = 2; i < LD.line.size(); ++i)
        {
            lastId = endId;
            endId = INVALID_ID;
            castRay(m_points[lastId],LD.line[i],LD.id,lastId,endId);//not snapped here!
        }
        if(LD.isLooped)
            castRay(m_points[endId],m_points[startId],LD.id,endId,startId);
        return;
    }
    */


/*
//This stuff is interesting but should I just remove it...
void QuadTreeTesselator::insertNewLine(const Point &a, const Point &b, ID id)
{
    m_startPtId = INVALID_ID;
    m_lastPtId = INVALID_ID;
    castRay(a,b,id,m_startPtId,m_lastPtId);
}

void QuadTreeTesselator::continueLine(const Point &b, ID id)
{
    PointId startId = m_lastPtId;
    m_lastPtId = INVALID_ID;
    castRay(m_points[startId],b,id,startId, m_lastPtId);//fill m_lastPt with what was last
}

void QuadTreeTesselator::loopLine(ID id)
{
    castRay(m_points[m_lastPtId],m_points[m_startPtId],id,m_lastPtId,m_startPtId);
}
*/