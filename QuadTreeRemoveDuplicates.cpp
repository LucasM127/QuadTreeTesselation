#include "QuadTreeTesselator.hpp"
#include <numeric>
#include <algorithm>

struct SortXYIndices
{
    SortXYIndices(const std::vector<Point> &_points) : points(_points) {}
    const std::vector<Point> &points;
    bool operator()(const int a, const int b) const
    {
        if(points[a].x == points[b].x)
            return(points[a].y < points[b].y);
        return(points[a].x < points[b].x);
    }
};

static std::unordered_map<unsigned int, unsigned int> getDuplicateMap(const std::vector<Point> &points, unsigned int startIndex)
{
    std::unordered_map<unsigned int, unsigned int> removedIndiceMap;

    size_t sz = points.size() - startIndex;
    if(sz == 0)
        return removedIndiceMap;
    
    std::vector<unsigned int> indices(sz);
    std::iota(indices.begin(),indices.end(),startIndex);

    SortXYIndices sorter(points);
    std::sort(indices.begin(),indices.end(),sorter);

    unsigned int i_last = indices[0];
    for(size_t k = 1; k < indices.size(); ++k)
    {
        const unsigned int i = indices[k];
        const Point &p = points[i];
        const Point &p_last = points[i_last];
        if(p == p_last)
            removedIndiceMap[i] = i_last;
        else
            i_last = i;
    }

    return removedIndiceMap;
}

namespace TESS
{

//Called after genGridPoints and genLinePoints //Kinda annoying
void QuadTreeTesselator::removeDuplicateLinePoints()
{
    std::unordered_map<ID,ID> duplicateMap = getDuplicateMap(m_points, m_numGridPoints);

    for(auto &pnode : m_lineNodes)
    {
        CellInfo &C = m_cellInfos[pnode->id];
        for(auto &i : C.linePointIds)
            if(i > m_numGridPoints && duplicateMap.find(i) != duplicateMap.end())
                i = duplicateMap[i];
    }
}

} //namespace TESS