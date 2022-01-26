#include "DQT.hpp"

//#define DQT_WRAP_NEIGHBOURS
//#define DQT_POINTS_ON_CORNERS_ONLY
//#define DQT_UNBALANCED_TREE

namespace DQT
{

const ID INVALID_ID = -1;
//Don't use mask if want neighbours to wrap around on findNeighbours function
const ID OUT_OF_BOUNDS_MASK = 0b11000000000000000000000000000000;

Node::Node(ID _id, ID _dirVec, uint8_t _depth, bool _isLeaf) : id(_id), dirVec(_dirVec), depth(_depth), isLeaf(_isLeaf)
{}

static ID neighbourDirVec(ID k, int depth, DIR dir)
{
    //http://www.lcad.icmc.usp.br/~jbatista/procimg/quadtree_neighbours.pdf
    const ID ty = 2863311530;//1010...1010
    const ID tx = 1431655765;//0101...0101

    ID N = 0;

    if(dir == DIR::NORTH)//north neighbour
        N = 2 << (30 - 2 * depth);
    else if(dir == DIR::EAST)//east neighbour
        N = 1 << (30 - 2*depth);
    else if(dir == DIR::SOUTH)//south neighbour
        N = ty << (30 - 2*depth);
    else if(dir == DIR::WEST)//West neighbour
        N = tx << (30 - 2*depth);

    ID kn = (((k|ty)+(N&tx))&tx) | (((k|tx)+(N&ty))&ty);

    return kn;
}

QuadTree::QuadTree(int min_x, int max_x, int min_y, int max_y) : m_numLeaves(0), m_nullNode(INVALID_ID,INVALID_ID,0,false)
{
    int w = max_x - min_x;
    int h = max_y - min_y;
    int depth = 0;
    int sz = 1;

    while (sz < w || sz < h)
    {
        sz *= 2;
        ++depth;
    }
    m_maxDepth = depth;
    m_maxSz = sz;
    
    m_nodes.emplace_back(m_numLeaves,0,0,true);
    ++m_numLeaves;
}

const int QuadTree::numLeaves() const
{
    return m_numLeaves;
}

const Node &QuadTree::sw(const Node &node) const
{
    assert(!node.isLeaf);
    return m_nodes[node.id];
}

const Node &QuadTree::se(const Node &node) const
{
    assert(!node.isLeaf);
    return m_nodes[node.id+1];
}

const Node &QuadTree::nw(const Node &node) const
{
    assert(!node.isLeaf);
    return m_nodes[node.id+2];
}

const Node &QuadTree::ne(const Node &node) const
{
    assert(!node.isLeaf);
    return m_nodes[node.id+3];
}

const Node &QuadTree::root() const
{
    return m_nodes[0];
}

//Node containing a point
const Node &QuadTree::at(int px, int py) const
{
    NodeId nodeId = _at(px,py);
    if(nodeId == INVALID_ID)
        return m_nullNode;
    return m_nodes[nodeId];
}

//Node a ray is entering
const Node &QuadTree::at(float x, float y, bool headsRight, bool headsUp) const
{
    bool skipLeft = false;
    bool skipDown = false;
    int px = x;
    int py = y;

    if(x == static_cast<float>(px) && !headsRight)
        skipLeft = true;
    if(y == static_cast<float>(py) && !headsUp)
        skipDown = true;
    
    if((!headsRight) && skipLeft)
        --px;
    if((!headsUp) && skipDown)
        --py;

    return at(px,py);
}

const Node &QuadTree::neighbour(const Node &node, DIR dir) const
{
    ID dirVec = neighbourDirVec(node.dirVec, node.depth, dir);
#ifndef DQT_WRAP_NEIGHBOURS
    if(dirVec & OUT_OF_BOUNDS_MASK)
        return m_nullNode;
#endif
    return atDir(dirVec, node.depth);
}

Box QuadTree::calcBounds(const Node& node) const
{
    int x = 0;
    int y = 0;
    int sz = m_maxSz;

    for(int i = 0; i < node.depth; ++i)
    {
        sz = sz/2;
        int k = (node.dirVec >> (28-2*i)) & 3;
        if(k & 1) x += sz;
        if(k & 2) y += sz;
    }
    return {x,y,sz};
}


const Node &QuadTree::atDir(ID dirVec, int depth) const
{
    NodeId nodeId = 0;
    for(int d = 0; d < depth; ++d)
    {
        if(m_nodes[nodeId].isLeaf)
            break;
        int o = (28-2*d);
        int k = (dirVec & (3 << o)) >> o;
        nodeId = m_nodes[nodeId].id + k;
    }
    return m_nodes[nodeId];
}

NodeId QuadTree::_at(int px, int py, int targetDepth) const
{
    NodeId nodeId = 0;
    int sz = m_maxSz;
    int nx = 0;
    int ny = 0;
    
    if(px >= sz || py >= sz || px < 0 || py < 0)
        return INVALID_ID;

    int depth = 0;
        
    while(!m_nodes[nodeId].isLeaf && depth < targetDepth)
    {
        sz /= 2;
        int k = 0;

        if((px - nx) >= sz)
        {
            nx += sz;
            k |= 1;
        }
        if((py - ny) >= sz)
        {
            k |= 2;
            ny += sz;
        }
        
        nodeId = m_nodes[nodeId].id + k;
        ++depth;
    }

    return nodeId;
}

NodeId QuadTree::_atDir(ID dirVec, int depth) const
{
    NodeId nodeId = 0;
    for(int d = 0; d < depth; ++d)
    {
        assert(m_nodes[nodeId].isLeaf == false);//don't want to take child of a leaf
        int o = (28-2*d);
        int k = (dirVec & (3 << o)) >> o;
        nodeId = m_nodes[nodeId].id + k;
    }
    return nodeId;
}

void QuadTree::subdivide(int px, int py)
{
    NodeId nodeId = _at(px,py);

    if(nodeId == INVALID_ID)
        return;

    assert(m_nodes[nodeId].isLeaf == true);

    Box n = calcBounds(m_nodes[nodeId]);
    
    int modx = px % n.sz;
    int mody = py % n.sz;
#ifdef DQT_POINTS_ON_CORNERS_ONLY
    while(modx != 0 || mody != 0)
#else
    while(modx != 0 && mody != 0)
#endif
    {
#ifndef DQT_UNBALANCED_TREE
        balanceNeighbours(nodeId,DIR::NONE);
#endif
        subdivide(nodeId);

        n.sz /= 2;
        int k = 0;
        if((px - n.x) >= n.sz)
        {
            k |= 1;
            n.x += n.sz;
        }
        if((py - n.y) >= n.sz)
        {
            k |= 2;
            n.y += n.sz;
        }

        nodeId = m_nodes[nodeId].id + k;

        modx = px % n.sz;
        mody = py % n.sz;
    }
}

void QuadTree::subdivide(NodeId nodeId)
{
    assert(m_nodes[nodeId].isLeaf == true);
    m_nodes[nodeId].isLeaf = false;
    int depth = m_nodes[nodeId].depth;
    int oldDataId = m_nodes[nodeId].id;
    m_nodes[nodeId].id = m_nodes.size();

    m_nodes.emplace_back(oldDataId,   m_nodes[nodeId].dirVec,                       m_nodes[nodeId].depth+1, true);
    m_nodes.emplace_back(m_numLeaves, m_nodes[nodeId].dirVec | 1 << (28 - 2*depth), m_nodes[nodeId].depth+1, true);
    ++m_numLeaves;
    m_nodes.emplace_back(m_numLeaves, m_nodes[nodeId].dirVec | 2 << (28 - 2*depth), m_nodes[nodeId].depth+1, true);
    ++m_numLeaves;
    m_nodes.emplace_back(m_numLeaves, m_nodes[nodeId].dirVec | 3 << (28 - 2*depth), m_nodes[nodeId].depth+1, true);
    ++m_numLeaves;
}

//~75% of this QuadTree Algorithm's time is spent here
void QuadTree::balanceNeighbours(NodeId nodeId, DIR skipDir)
{
    int depth = m_nodes[nodeId].depth;
    ID dirVec = m_nodes[nodeId].dirVec;

#ifdef DQT_WRAP_NEIGHBOURS
    if(depth == 0) return;
#endif

    bool isEast  = dirVec & (1 << (30-2*depth));
    bool isNorth = dirVec & (2 << (30-2*depth));

    if(skipDir != DIR::NORTH && isNorth)
    {
        ID northDirVec = neighbourDirVec(dirVec, depth,DIR::NORTH);
#ifndef DQT_WRAP_NEIGHBOURS
        if(northDirVec & OUT_OF_BOUNDS_MASK) goto NorthExit;
#endif
        int northNodeId = _atDir(northDirVec, depth-1);
        if(m_nodes[northNodeId].isLeaf)
        {
            subdivide(northNodeId);
            balanceNeighbours(northNodeId,DIR::SOUTH);
        }
    }
    NorthExit:
    if(skipDir != DIR::EAST && isEast)
    {
        ID eastDirVec = neighbourDirVec(dirVec, depth, DIR::EAST);
#ifndef DQT_WRAP_NEIGHBOURS
        if(eastDirVec & OUT_OF_BOUNDS_MASK) goto EastExit;
#endif
        int eastNodeId = _atDir(eastDirVec, depth-1);
        if(m_nodes[eastNodeId].isLeaf)
        {
            subdivide(eastNodeId);
            balanceNeighbours(eastNodeId,DIR::WEST);
        }
    }
    EastExit:
    if(skipDir != DIR::SOUTH && !isNorth)
    {
        ID southDirVec = neighbourDirVec(dirVec, depth, DIR::SOUTH);
#ifndef DQT_WRAP_NEIGHBOURS
        if(southDirVec & OUT_OF_BOUNDS_MASK) goto SouthExit;
#endif
        int southNodeId = _atDir(southDirVec, depth-1);
        if(m_nodes[southNodeId].isLeaf)
        {
            subdivide(southNodeId);
            balanceNeighbours(southNodeId,DIR::NORTH);
        }
    }
    SouthExit:
    if(skipDir != DIR::WEST && !isEast)
    {
        ID westDirVec = neighbourDirVec(dirVec, depth, DIR::WEST);
#ifndef DQT_WRAP_NEIGHBOURS
        if(westDirVec & OUT_OF_BOUNDS_MASK) return;
#endif
        int westNodeId = _atDir(westDirVec, depth-1);
        if(m_nodes[westNodeId].isLeaf)
        {
            subdivide(westNodeId);
            balanceNeighbours(westNodeId,DIR::EAST);
        }
    }
}

}// namespace DQT