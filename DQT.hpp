#ifndef DISCRETIZED_QUADTREE_HPP
#define DISCRETIZED_QUADTREE_HPP

#include <vector>
#include <cassert>
#include "Consts.hpp"

namespace TESS
{

struct Node
{
    Node(ID _id, ID _dirVec, uint8_t _depth, bool _isLeaf,
         ID northNeighbour = INVALID_ID, ID eastNeighbour = INVALID_ID, ID southNeighbour = INVALID_ID, ID westNeighbour = INVALID_ID);
    ID id;
    ID dirVec;
    uint8_t depth;
    bool isLeaf;
    ID neighbours[4];
};

struct Box
{
    int x, y, sz;
};

class QuadTree
{
public:
    QuadTree(int min_x, int max_x, int min_y, int max_y);
    void subdivide(int px, int py);

    const Node &at(int px, int py) const;
    const Node &at(float x, float y, bool headsRight, bool headsUp) const;
    const Node &neighbour(const Node &node, DIR dir) const;
    const Node &sw(const Node &node) const;
    const Node &se(const Node &node) const;
    const Node &nw(const Node &node) const;
    const Node &ne(const Node &node) const;
    const Node &root() const;
    Box calcBounds(const Node &node) const;
    const int numLeaves() const;
private:
    int m_numLeaves;
    int m_maxDepth, m_maxSz;
    std::vector<Node> m_nodes;
    Node m_nullNode;
    
    NodeId _at(int px, int py, int targetDepth = 256) const;

    void subdivide(NodeId nodeId);
    void balanceNeighbours(NodeId nodeId, DIR skipDir);

    void updateNeighbours(ID nodeId, ID thisId, DIR neighbourSide);
};

}//namespace TESS

#endif //DISCRETIZED_QUADTREE_HPP