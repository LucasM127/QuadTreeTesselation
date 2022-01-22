#include "CellInfo.hpp"
#include "Quadrants.hpp"
#include <cassert>
#include <algorithm>
#include <array>

#include <stack>

//This function is the slowest part of the algorithm
//The range check part especially should be done another way

#define INCLUDE_STEINER_POINTS

namespace TESS
{

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

//naming?
struct Range
{
    Range(float f, int i) : df(f), id(i){}
    float df;
    int id;
};

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

//ignore flat triangles
bool isNiceTriangle(const Point &a, const Point &b, const Point &c)
{
    if(a==b || a==c || b==c)
        return false;
    return true;
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
            concavePolygons[ctr].push_back(activeList.back());
            activeList.pop_back();
        }
        concavePolygons[ctr].push_back(activeList.back());
        activeList.back().type = BPoint::TYPE::FREE;
        concavePolygons[ctr].push_back(B);
        activeList.push_back(B);
        ++ctr;
    }
    while(activeList.size())//last concave polygon
    {
        concavePolygons[ctr].push_back(activeList.back());
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

    //THIS part is overly unnecessarily complex but my brain is turning into mush so i'll let it sit for a bit
    //Having both sides of the polyline polygon ids predefined should hopefully make it easier though
    //Figure out range continuities
    std::vector<Range> ranges;
    {
    float f_last = 0.f;
    int ctr = 0;
    std::stack<std::vector<size_t>> prevIndicesStack;
    prevIndicesStack.push(std::vector<size_t>());
    
    for(auto p : pointList)
    {
        float f = mapToRange(p.p);
        if(p.type == BPoint::TYPE::START)
        {
            prevIndicesStack.top().push_back(ranges.size());
            ranges.emplace_back(f-f_last,ctr);
            prevIndicesStack.push({ranges.size()});
            f_last = f;
        }
        if(p.type == BPoint::TYPE::END)
        {
            ranges.emplace_back(f-f_last,ctr);//this one is correct
            for(size_t i : prevIndicesStack.top())
                ranges[i].id = ctr;
            prevIndicesStack.pop();
            f_last = f;
            ++ctr;//done the polygon
        }
    }
    //for the very last polygon... polygonX
    ranges.emplace_back(4.0f-f_last,ctr);
    for(size_t i : prevIndicesStack.top())
        ranges[i].id = ctr;
    //std::cout<<"After walking ranges are: \n";
    //for(auto r : ranges)
    //    std::cout<<"ID:"<<r.id<<" "<<r.df<<"\n"; 
    }

    //use ranges to map NeighbourIds
    {
    auto checkRangePolygonId = [](const std::vector<Range> &ranges, const std::vector<PointId> &polyIds, float lower, float upper) -> ID
    {
        float distTravelled = 0.f;
        auto it = ranges.begin();
        while(it != ranges.end())
        {
            if(distTravelled + it->df > lower)
                break;
            distTravelled += it->df;
            it = std::next(it);
        }
        PointId id = polyIds[it->id];
        while(it != ranges.end())
        {
            if(polyIds[it->id] != id)
                return INVALID_ID;
            if(distTravelled + it->df >= upper)
                break;
            distTravelled += it->df;
            it = std::next(it);
        }
        return id;
    };

    C.neighbourIds[DIR::UP]    = checkRangePolygonId(ranges,polygonIds,0,1);// getRangePolygonId(polygonIds,it1,it2);
    C.neighbourIds[DIR::LEFT]  = checkRangePolygonId(ranges,polygonIds,1,2);// getRangePolygonId(polygonIds,it2,it3);
    C.neighbourIds[DIR::DOWN]  = checkRangePolygonId(ranges,polygonIds,2,3);// getRangePolygonId(polygonIds,it3,it4);
    C.neighbourIds[DIR::RIGHT] = checkRangePolygonId(ranges,polygonIds,3,4);// getRangePolygonId(polygonIds,it4,it5);
    }

    //and finally fan create the triangles from the concave polygons
    {//If you want a prettier triangulation can implement delaunay here if you end up noticing it....
    unsigned int k = 0;
    for(auto &concavePolygon : concavePolygons)
    {
        ID triangleId = polygonIds[k];
        ++k;
        
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

} // namespace TESS

/*
Here is a test for isDelaunay for reference copy/pasted from another file
//https://www.newcastle.edu.au/__data/assets/pdf_file/0019/22519/23_A-fast-algortithm-for-generating-constrained-Delaunay-triangulations.pdf
bool TriangularMesh::isDelaunay(EdgeId A)
{//return true;
    const float epsilon = std::numeric_limits<float>::epsilon();
    EdgeId B = m_mates[A];
    if(B >= CEILING) return true;//on hull
    Point p1(points[m_edges[A]]);
    Point p2(points[m_edges[next(A)]]);
    Point p3(points[m_edges[last(A)]]);
    Point p(points[m_edges[last(B)]]);
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
    return rhs - lhs < epsilon;//0;//epsilon;//0;//-epsilon;
}//epsilon removes unnecessary flips if equidistant...
*/
/*
std::ostream &operator<<(std::ostream &os, const BPoint &B)
{
    os<<"ID:"<<B.p_id<<" ";
    switch (B.type)
    {
    case BPoint::TYPE::START:
        os<<"START";
        break;
    case BPoint::TYPE::END:
        os<<"END";
        break;
    case BPoint::TYPE::FREE:
        os<<"FREE";
        break;
    default:
        break;
    }
    return os << " " << B.p.x << " " << B.p.y;
}
*/

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


/*
bool addTriangle(std::vector<Point> &triangles, const Point &a, const Point &b, const Point &c)
{
    if(a==b || a==c || b==c)
        return false;
    triangles.push_back(a);
    triangles.push_back(b);
    triangles.push_back(c);
    return true;
}*/

/*
bool sortLines(const Line &l1, const Line &l2)
{
    if(l1.b == l2.b)
        return !isLessCCW(l1.a,l2.a);
    return isLessCCW(l1.b, l2.b);
}*/

/*  Seem to get bounds errors this way :(
    auto getRangeItAt = [](const std::vector<Range> &R, float f) -> auto
    {
        float distTravelled = 0.f;
        for(auto it = R.begin(); it != R.end(); it = std::next(it))
        {
            if(distTravelled + it->df > f)//> not >= ???
                return it;
            distTravelled += it->df;
        }
        return R.end();
    };

    auto getRangePolygonId = [](const std::vector<PointId> &polyIds, std::vector<Range>::const_iterator it1, std::vector<Range>::const_iterator it2) -> ID
    {
        PointId id = polyIds[it1->id];//????
        for(auto it = std::next(it1); it != std::next(it2); it = std::next(it))
        {
            if(it->df != 0.f && polyIds[it->id] != id)
                return INVALID_ID;
        }
        return id;
    };
*/