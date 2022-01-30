#include "CellInfo.hpp"
#include "Quadrants.hpp"
#include <cassert>
#include <algorithm>

#include "Interval.hpp"

//This function is the slowest part of the algorithm
//parallellizable

#define INCLUDE_STEINER_POINTS

namespace TESS
{

const ID INVALID_ID = -1;
const ID EMPTY_SPACE_ID = 0;//INVALID_ID - 1;

//https://www.newcastle.edu.au/__data/assets/pdf_file/0019/22519/23_A-fast-algortithm-for-generating-constrained-Delaunay-triangulations.pdf
bool isDelaunay(const Point &p1, const Point &p2, const Point &p3, const Point &p)
{//return true;
    const float epsilon = std::numeric_limits<float>::epsilon();
//is d in circumradius of triangle abc ???
    float x13 = p1.x - p3.x;
    float y13 = p1.y - p3.y;
    float x23 = p2.x - p3.x;
    float y23 = p2.y - p3.y;
    float x1p = p1.x - p.x;
    float y1p = p1.y - p.y;
    float x2p = p2.x - p.x;
    float y2p = p2.y - p.y;

    float lhs = (x13*x23 + y13*y23)*(x2p*y1p - x1p*y2p);
    float rhs = (x23*y13 - x13*y23)*(x2p*x1p + y1p*y2p);
    return rhs - lhs < epsilon;
}//epsilon removes unnecessary flips if equidistant...

/*********************
 *    (1) <--- (4)
 *     a________d
 *  |  |        | ^
 *  |  |        | |
 *  V  |        | |
 *     b________c
 *    (2) ---> (3)
 *********************/
//(0-4]
float mapToRange(const Point &p)
{
    assert(p.x == 1.f || p.x == 0.f || p.y == 1.f || p.y == 0.f);
    if(p.x == 1.f)
        return 3.f + p.y;
    if(p.y == 1.f)
        return 1.f - p.x;
    if(p.x == 0.f)
        return 2.f - p.y;
    return 2.f + p.x;
}

bool isLessCCW(const Point &a, const Point &b)
{
    return mapToRange(a) < mapToRange(b);
}

struct BPoint
{
    enum class TYPE
    {
        START,
        END,
        FREE
    };
    BPoint(const Point _p, TYPE _type, PointId _p_id)
            : p(_p), type(_type), p_id(_p_id) {}
    static bool sort(const BPoint &a, const BPoint &b)
    {
        float ra = mapToRange(a.p);
        float rb = mapToRange(b.p);
        if(ra == rb)//duplicate points
        {//endPoints - cornerPt - startPts
            if(a.type == TYPE::START)
                return false;
            if(a.type == TYPE::END)
                return true;
            if(b.type == TYPE::START)
                return true;
            if(b.type == TYPE::END)
                return false;
            throw;//duplicate corner points not allowed
        }
        return ra < rb;
    }
    Point p;
    TYPE type;
    PointId p_id;
};

struct Line
{
    Line(const Point &a_, const Point &b_, PointId a_id_, PointId b_id_, unsigned int idr_, unsigned int idl_, bool flipped)
        : a(a_), b(b_), a_id(a_id_), b_id(b_id_), rightPolygonId(idr_), leftPolygonId(idl_), isFlipped(flipped) {}
    Point a,b;
    PointId a_id, b_id;
    ID rightPolygonId, leftPolygonId;
    bool isFlipped;

    //sort lines by the order that the boundary walk will 'finish' the line and make a polygon out of it
    //if both finish at the same point, the line started later makes the first polygon
    static bool sort(const Line &l1, const Line &l2)
    {
        if(l1.b == l2.b)
            return !isLessCCW(l1.a,l2.a);
        return isLessCCW(l1.b, l2.b);
    }
};

bool notADuplicatePoint(const Point &a, const Point &b)
{
    return !(a==b);
}

void addIfNotDuplicate(std::vector<BPoint> &concavePolygon, const BPoint &B)
{
    if(concavePolygon.size() == 0)
    {
        concavePolygon.push_back(B);
        return;
    }
    if(notADuplicatePoint(concavePolygon.back().p, B.p))
        concavePolygon.push_back(B);
}

void triangulateCellNode(CellInfo &C)
{
    std::vector<BPoint> pointList =
    {
        {{0.f,1.f}, BPoint::TYPE::FREE, C.cornerIds[QUADRANT::NW]},
        {{0.f,0.f}, BPoint::TYPE::FREE, C.cornerIds[QUADRANT::SW]},
        {{1.f,0.f}, BPoint::TYPE::FREE, C.cornerIds[QUADRANT::SE]},
        {{1.f,1.f}, BPoint::TYPE::FREE, C.cornerIds[QUADRANT::NE]}
    };
#ifdef INCLUDE_STEINER_POINTS
    if(C.steinerIds[DIR::UP] != INVALID_ID)
        pointList.push_back({{.5f,1.f}, BPoint::TYPE::FREE, C.steinerIds[DIR::UP]});
    if(C.steinerIds[DIR::LEFT] != INVALID_ID)
        pointList.push_back({{0.f,.5f}, BPoint::TYPE::FREE, C.steinerIds[DIR::LEFT]});
    if(C.steinerIds[DIR::DOWN] != INVALID_ID)
        pointList.push_back({{.5f,0.f}, BPoint::TYPE::FREE, C.steinerIds[DIR::DOWN]});
    if(C.steinerIds[DIR::RIGHT] != INVALID_ID)
        pointList.push_back({{1.f,.5f}, BPoint::TYPE::FREE, C.steinerIds[DIR::RIGHT]});
#endif

    std::vector<Line> lines;
    lines.reserve(C.lines.size()/2);

    //create the lines
    for(size_t i = 0; i < C.lines.size()/2; ++i)
    {
        const Point &a = C.lines[2*i];
        const Point &b = C.lines[2*i+1];
        PointId a_id = C.linePointIds[2*i];
        PointId b_id = C.linePointIds[2*i+1];
        ID rightPolygonId = C.linePolygonIds[2*i];
        ID leftPolygonId = C.linePolygonIds[2*i+1];

        if(isLessCCW(a,b))
            lines.emplace_back(a, b, a_id, b_id, rightPolygonId, leftPolygonId, false);
        else
            lines.emplace_back(b, a, b_id, a_id, rightPolygonId, leftPolygonId, true);
    }//This might be redundant
    std::sort(lines.begin(),lines.end(),Line::sort);

    for(auto &l : lines)
    {
        pointList.emplace_back(l.a, BPoint::TYPE::START, l.a_id);
        pointList.emplace_back(l.b, BPoint::TYPE::END,   l.b_id);
    }
    std::sort(pointList.begin(),pointList.end(), BPoint::sort);

    //boundary walk... creating concave polygon lists
    std::vector<BPoint> activeList;
    activeList.reserve(8);
    std::vector<std::vector<BPoint>> concavePolygons(lines.size() + 1);
    for(auto &v : concavePolygons) v.reserve(8);
    unsigned int ctr = 0;
    for(const auto &B : pointList)
    {
        if(B.type != BPoint::TYPE::END)
        {
            activeList.push_back(B);
            continue;
        }

        while(activeList.back().type != BPoint::TYPE::START)
        {
            addIfNotDuplicate(concavePolygons[ctr],activeList.back());
            activeList.pop_back();
        }
        addIfNotDuplicate(concavePolygons[ctr],activeList.back());
        addIfNotDuplicate(concavePolygons[ctr],B);
        activeList.back().type = BPoint::TYPE::FREE;
        activeList.push_back(B);
        ++ctr;
    }
    while(activeList.size())//last concave polygon
    {
        addIfNotDuplicate(concavePolygons[ctr],activeList.back());
        activeList.pop_back();
    }

    //map polygonIds
    std::vector<ID> polygonIds(concavePolygons.size(), EMPTY_SPACE_ID);//'empty' space default
    for(size_t i = 0; i < lines.size(); ++i)//for each line
    {
        if(!lines[i].isFlipped)
            polygonIds[i] = lines[i].rightPolygonId;
        else
            polygonIds[i] = lines[i].leftPolygonId;
    }
    //and then for the last of the polygonIds, polygonX if you will
    if(lines.back().isFlipped)
        polygonIds.back() = lines.back().rightPolygonId;
    else
        polygonIds.back() = lines.back().leftPolygonId;

    //Figure out range continuities
    {
    std::vector<Intervali> intervals;
    intervals.reserve(8);
    intervals.emplace_back(0.f,4.f,polygonIds.back());
    for(size_t i = 0; i < lines.size(); ++i)
    {
        Intervali I(mapToRange(lines[i].a), mapToRange(lines[i].b), polygonIds[i]);
        insert(I, intervals);
    }
    std::vector<Intervali> topInterval    = queryInterval(0.f,1.f,intervals);
    std::vector<Intervali> leftInterval   = queryInterval(1.f,2.f,intervals);
    std::vector<Intervali> bottomInterval = queryInterval(2.f,3.f,intervals);
    std::vector<Intervali> rightInterval  = queryInterval(3.f,4.f,intervals);

    auto isContinuous = [](const std::vector<Intervali> &intervals) -> bool
    {
        ID i = intervals.front().val;
        for(auto it = std::next(intervals.begin()); it != intervals.end(); it = std::next(it))
            if(it->val != i)
                return false;
        return true;
    };
    
    if(isContinuous(topInterval))
        C.neighbourIds[DIR::UP] = topInterval.front().val;
    if(isContinuous(leftInterval))
        C.neighbourIds[DIR::LEFT] = leftInterval.front().val;
    if(isContinuous(bottomInterval))
        C.neighbourIds[DIR::DOWN] = bottomInterval.front().val;
    if(isContinuous(rightInterval))
        C.neighbourIds[DIR::RIGHT] = rightInterval.front().val;
    }

    //fan delaunay triangulate. Think I got it this time. (fingers crossed)
    //TODO... maybe make a visualizer of this algorithm so I can understand it better / test robustness
    //is better than it was though so that's good
    {
    unsigned int k = 0;
    for(auto &concavePolygon : concavePolygons)
    {
        ID triangleId = polygonIds[k];
        ++k;
        
        const BPoint *P3 = &concavePolygon[1];//i-2 when i = 3
        const BPoint *P3last = P3;
        size_t editPtIndex;
        for(size_t i = 3; i < concavePolygon.size(); ++i)//only delaunay test if 4 points
        {//p3 is FAR point = i-2
            if(isDelaunay(concavePolygon[0].p, concavePolygon[i-1].p, P3->p, concavePolygon[i].p))
            {//0 -> (i-2) -> (i-1) //i-1 is p3 future
                editPtIndex = C.triangles.size();
                C.triangles.push_back(concavePolygon[0].p_id);
                C.triangles.push_back(P3->p_id);//p3
                C.triangles.push_back(concavePolygon[i-1].p_id);
                P3last = P3;//???
                P3 = &concavePolygon[i-1];
            }
            else
            {//(i-2)->(i-1)->i  but i-2 is p3 future so unchanged
                C.triangles.push_back(P3->p_id);
                C.triangles.push_back(concavePolygon[i-1].p_id);
                C.triangles.push_back(concavePolygon[i].p_id);//that's right
            }
            C.triangleIds.push_back(triangleId);
        }//which will leave 1 triangle in the queue at the end to pop out 0-(i-2)(OR P3)-i(now the back)
        if(concavePolygon.size() > 2)
        {
            //Test the last line delaunay with last from flipping it if there was a flip. (drawing pics helps my brain)
            if(P3last != P3  && !isDelaunay(concavePolygon[0].p, concavePolygon.back().p, P3->p, P3last->p))
            {
                    C.triangles.push_back(concavePolygon.back().p_id);// P3last->p_id);
                    C.triangles.push_back(P3last->p_id);//P3->p_id);
                    C.triangles.push_back(concavePolygon[0].p_id);//concavePolygon.back().p_id);
                    C.triangleIds.push_back(triangleId);
                    //edit the last triangle too!!!
                    C.triangles[editPtIndex] = concavePolygon.back().p_id;//yes
            }
            else
            {
                C.triangles.push_back(concavePolygon[0].p_id);
                C.triangles.push_back(P3->p_id);
                C.triangles.push_back(concavePolygon.back().p_id);
                C.triangleIds.push_back(triangleId);
            }
        }
    }
    }

}

//Either edge 0-(i-1) is delaunay or not becoming these 2 cases... 0-i becomes future 0-(i-1) edge in BOTH cases
/***********************************
 * 
 *            0                    0
 *           /|\                  / \
 *          / | \                /   \
 *         /  |  \         (i-2)/_____\(i)
 *        / A | B \             \     /
 *        --\_|_/--              \   /
 *   (i-2)        (i)             \ /
 *          (i-1)                (i-1)
 *************************************/

} // namespace TESS

/*
//THERE IS A BUG IN HERE TOO
    C.neighbourIds[0] = C.neighbourIds[1] = C.neighbourIds[2] = C.neighbourIds[3] = INVALID_ID;
    unsigned int k = 0;
    bool abWasSet = false;
    bool bcWasSet = false;
    bool cdWasSet = false;
    bool daWasSet = false;

    for(auto &concavePolygon : concavePolygons)
    {
        ID triangleId = polygonIds[k++];

        bool a = false;//top left
        bool b = false;//bottom left
        bool c = false;//bottom right
        bool d = false;//top right

        for(unsigned int i = 0; i < concavePolygon.size(); ++i)
        {
            if(concavePolygon[i].p == Point(0,1))// .p_id == C.cornerIds[QUADRANT::NW])//Point(0,1))
                a = true;
            if(concavePolygon[i].p == Point(0,0))//p_id == C.cornerIds[QUADRANT::SW])//Point(0,0))
                b = true;
            if(concavePolygon[i].p == Point(1,0))//p_id == C.cornerIds[QUADRANT::SE])//Point(1,0))
                c = true;
            if(concavePolygon[i].p == Point(1,1))//p_id == C.cornerIds[QUADRANT::NE])//Point(1,1))
                d = true;
        }
        if(a && b && !abWasSet)
        {
            C.neighbourIds[CellNeighbour::LEFT] = triangleId;
            abWasSet = true;
        }
        if(b && c && !bcWasSet)
        {
            C.neighbourIds[CellNeighbour::DOWN] = triangleId;
            bcWasSet = true;
        }
        if(c && d && !cdWasSet)
        {
            C.neighbourIds[CellNeighbour::RIGHT] = triangleId;
            cdWasSet = true;
        }
        if(d && a && !daWasSet)
        {
            C.neighbourIds[CellNeighbour::UP] = triangleId;
            daWasSet = true;
        }

        //create our triangles...
        {
        for(size_t i = 2; i < concavePolygon.size(); ++i)
        {
            if(isNiceTriangle(concavePolygon[0].p, concavePolygon[i-1].p, concavePolygon[i].p))
            {
                C.triangles.push_back(concavePolygon[0].p_id);
                C.triangles.push_back(concavePolygon[i-1].p_id);
                C.triangles.push_back(concavePolygon[i].p_id);
                C.triangleIds.push_back(triangleId);
            }
        }
        }
    }
    }
}*/