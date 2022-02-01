#include "DQT.hpp"

//#define DQT_WRAP_NEIGHBOURS
//#define DQT_POINTS_ON_CORNERS_ONLY
//#define DQT_UNBALANCED_TREE

namespace TESS
{

Node::Node(ID _id, ID _dirVec, uint8_t _depth, bool _isLeaf, ID northNeighbour, ID eastNeighbour, ID southNeighbour, ID westNeighbour)
          : id(_id), dirVec(_dirVec), depth(_depth), isLeaf(_isLeaf),
            neighbours {northNeighbour,eastNeighbour,southNeighbour,westNeighbour}
{}

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

#ifdef DQT_WRAP_NEIGHBOURS
    m_nodes.emplace_back(m_numLeaves,0,0,true,0,0,0,0);//self referential
#else
    m_nodes.emplace_back(m_numLeaves,0,0,true);
#endif
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
    if(node.neighbours[dir] == INVALID_ID)
        return m_nullNode;
    return m_nodes[node.neighbours[dir]];
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
        subdivide(nodeId);

#ifndef DQT_UNBALANCED_TREE
        balanceNeighbours(nodeId,DIR::NONE);
#endif

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

void QuadTree::updateNeighbours(ID nodeId, ID thisId, DIR neighbourSide)
{
    m_nodes[nodeId].neighbours[neighbourSide] = thisId;
    if(m_nodes[nodeId].isLeaf)
        return;
    switch (neighbourSide)
    {
    case DIR::NORTH:
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::NW, thisId, neighbourSide);
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::NE, thisId, neighbourSide);
        break;
    case DIR::EAST:
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::NE, thisId, neighbourSide);
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::SE, thisId, neighbourSide);
        break;
    case DIR::SOUTH:
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::SW, thisId, neighbourSide);
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::SE, thisId, neighbourSide);
        break;
    case DIR::WEST:
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::NW, thisId, neighbourSide);
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::SW, thisId, neighbourSide);
        break;
    default:
        break;
    }
};

void QuadTree::subdivide(NodeId nodeId)//SW SE NW NE //neighbours N E S W
{
    assert(m_nodes[nodeId].isLeaf == true);
    m_nodes[nodeId].isLeaf = false;//make it then update it????????
    int depth = m_nodes[nodeId].depth;
    int oldDataId = m_nodes[nodeId].id;
    ID childId = m_nodes.size();
    m_nodes[nodeId].id = childId;//m_nodes.size();
    ID northId = m_nodes[nodeId].neighbours[0];
    ID eastId  = m_nodes[nodeId].neighbours[1];
    ID southId = m_nodes[nodeId].neighbours[2];
    ID westId  = m_nodes[nodeId].neighbours[3];

    auto updateNeighbour = [this](ID neighbourId, ID thisId, QUADRANT neighbourQuadrant, DIR neighbourSide) -> ID
    {
        if(neighbourId == INVALID_ID)
            return INVALID_ID;
        if(m_nodes[neighbourId].isLeaf)
            return neighbourId;
        
        updateNeighbours(m_nodes[neighbourId].id+neighbourQuadrant, thisId, neighbourSide);

        return m_nodes[neighbourId].id+neighbourQuadrant;
    };
    
    //SW
    m_nodes.emplace_back(oldDataId,   m_nodes[nodeId].dirVec,                       m_nodes[nodeId].depth+1, true,
                         childId + QUADRANT::NW,//N
                         childId + QUADRANT::SE,//E
                         INVALID_ID,//updateNeighbour(southId, childId+0, QUADRANT::NW, DIR::NORTH),//S
                         INVALID_ID);//updateNeighbour(westId,  childId+0, QUADRANT::SE, DIR::EAST));//W
    //SE
    m_nodes.emplace_back(m_numLeaves, m_nodes[nodeId].dirVec | 1 << (28 - 2*depth), m_nodes[nodeId].depth+1, true,
                         childId + QUADRANT::NE,//N
                         INVALID_ID,//updateNeighbour(eastId,  childId+1, QUADRANT::SW, DIR::WEST),//E
                         INVALID_ID,//updateNeighbour(southId, childId+1, QUADRANT::NE, DIR::NORTH),//S
                         childId + QUADRANT::SW);//W
    ++m_numLeaves;
    //NW
    m_nodes.emplace_back(m_numLeaves, m_nodes[nodeId].dirVec | 2 << (28 - 2*depth), m_nodes[nodeId].depth+1, true,
                         INVALID_ID,//updateNeighbour(northId, childId+2, QUADRANT::SW, DIR::SOUTH),//N
                         childId + QUADRANT::NE,//E
                         childId + QUADRANT::SW,//S
                         INVALID_ID);//updateNeighbour(westId,  childId+2, QUADRANT::NE, DIR::EAST));//W
    ++m_numLeaves;
    //NE
    m_nodes.emplace_back(m_numLeaves, m_nodes[nodeId].dirVec | 3 << (28 - 2*depth), m_nodes[nodeId].depth+1, true,
                         INVALID_ID,//updateNeighbour(northId, childId+3, QUADRANT::SE, DIR::SOUTH),//N
                         INVALID_ID,//updateNeighbour(eastId,  childId+3, QUADRANT::NW, DIR::WEST),//E
                         childId + QUADRANT::SE,//S
                         childId + QUADRANT::NW);//W
    ++m_numLeaves;

//can't call till the children are made if want neighbour wrapping to work as updateNeighbour updates the children too...
    m_nodes[childId + QUADRANT::SW].neighbours[DIR::SOUTH] = updateNeighbour(southId, childId+0, QUADRANT::NW, DIR::NORTH);//S
    m_nodes[childId + QUADRANT::SW].neighbours[DIR::WEST]  = updateNeighbour(westId,  childId+0, QUADRANT::SE, DIR::EAST);//W
    m_nodes[childId + QUADRANT::SE].neighbours[DIR::EAST]  = updateNeighbour(eastId,  childId+1, QUADRANT::SW, DIR::WEST);//E
    m_nodes[childId + QUADRANT::SE].neighbours[DIR::SOUTH] = updateNeighbour(southId, childId+1, QUADRANT::NE, DIR::NORTH);//S
    m_nodes[childId + QUADRANT::NW].neighbours[DIR::NORTH] = updateNeighbour(northId, childId+2, QUADRANT::SW, DIR::SOUTH);//N
    m_nodes[childId + QUADRANT::NW].neighbours[DIR::WEST]  = updateNeighbour(westId,  childId+2, QUADRANT::NE, DIR::EAST);//W
    m_nodes[childId + QUADRANT::NE].neighbours[DIR::NORTH] = updateNeighbour(northId, childId+3, QUADRANT::SE, DIR::SOUTH);//N
    m_nodes[childId + QUADRANT::NE].neighbours[DIR::EAST]  = updateNeighbour(eastId,  childId+3, QUADRANT::NW, DIR::WEST);//E
}

//FASTER???????
//~75% of this QuadTree Algorithm's time is spent here
void QuadTree::balanceNeighbours(NodeId nodeId, DIR skipDir)
{
    int depth = m_nodes[nodeId].depth;
    ID dirVec = m_nodes[nodeId].dirVec;

    if(depth == 0) return;

    bool isEast  = dirVec & (1 << (30-2*depth));
    bool isNorth = dirVec & (2 << (30-2*depth));

    if(skipDir != DIR::NORTH && isNorth)
    {
        ID northNodeId = m_nodes[nodeId].neighbours[DIR::NORTH];
        if(northNodeId != INVALID_ID && m_nodes[northNodeId].isLeaf && m_nodes[northNodeId].depth < depth)
        {
            subdivide(northNodeId);
            balanceNeighbours(northNodeId,DIR::SOUTH);
        }
    }
    if(skipDir != DIR::EAST && isEast)
    {
        ID eastNodeId = m_nodes[nodeId].neighbours[DIR::EAST];
        if(eastNodeId != INVALID_ID && m_nodes[eastNodeId].isLeaf && m_nodes[eastNodeId].depth < depth)
        {
            subdivide(eastNodeId);
            balanceNeighbours(eastNodeId,DIR::WEST);
        }
    }
    if(skipDir != DIR::SOUTH && !isNorth)
    {
        ID southNodeId = m_nodes[nodeId].neighbours[DIR::SOUTH];//_atDir(southDirVec, depth-1);
        if(southNodeId != INVALID_ID && m_nodes[southNodeId].isLeaf && m_nodes[southNodeId].depth < depth)
        {
            subdivide(southNodeId);
            balanceNeighbours(southNodeId,DIR::NORTH);
        }
    }
    if(skipDir != DIR::WEST && !isEast)
    {
        ID westNodeId = m_nodes[nodeId].neighbours[DIR::WEST];//_atDir(westDirVec, depth-1);
        if(westNodeId != INVALID_ID && m_nodes[westNodeId].isLeaf && m_nodes[westNodeId].depth < depth)
        {
            subdivide(westNodeId);
            balanceNeighbours(westNodeId,DIR::EAST);
        }
    }
}

}// namespace TESS

/*

const ID INVALID_ID = -1;
//Don't use mask if want neighbours to wrap around on findNeighbours function
const ID OUT_OF_BOUNDS_MASK = 0b11000000000000000000000000000000;

Node::Node(ID _id, ID _dirVec, uint8_t _depth, bool _isLeaf, ID northNeighbour, ID eastNeighbour, ID southNeighbour, ID westNeighbour)
          : id(_id), dirVec(_dirVec), depth(_depth), isLeaf(_isLeaf),
            neighbours {northNeighbour,eastNeighbour,southNeighbour,westNeighbour}
{}
//obsoleted now ???
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

#ifdef DQT_WRAP_NEIGHBOURS
    m_nodes.emplace_back(m_numLeaves,0,0,true,0,0,0,0);//self referential
#else
    m_nodes.emplace_back(m_numLeaves,0,0,true);
#endif
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
    if(node.neighbours[dir] == INVALID_ID)
        return m_nullNode;
    return m_nodes[node.neighbours[dir]];

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
        subdivide(nodeId);

#ifndef DQT_UNBALANCED_TREE
        balanceNeighbours(nodeId,DIR::NONE);
#endif

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

void QuadTree::updateNeighbours(ID nodeId, ID thisId, DIR neighbourSide)
{
    m_nodes[nodeId].neighbours[neighbourSide] = thisId;
    if(m_nodes[nodeId].isLeaf)
        return;
    switch (neighbourSide)
    {
    case DIR::NORTH:
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::NW, thisId, neighbourSide);
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::NE, thisId, neighbourSide);
        break;
    case DIR::EAST:
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::NE, thisId, neighbourSide);
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::SE, thisId, neighbourSide);
        break;
    case DIR::SOUTH:
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::SW, thisId, neighbourSide);
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::SE, thisId, neighbourSide);
        break;
    case DIR::WEST:
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::NW, thisId, neighbourSide);
        updateNeighbours(m_nodes[nodeId].id + QUADRANT::SW, thisId, neighbourSide);
        break;
    default:
        break;
    }
};

void QuadTree::subdivide(NodeId nodeId)//SW SE NW NE //neighbours N E S W
{
    assert(m_nodes[nodeId].isLeaf == true);
    m_nodes[nodeId].isLeaf = false;//make it then update it????????
    int depth = m_nodes[nodeId].depth;
    int oldDataId = m_nodes[nodeId].id;
    ID childId = m_nodes.size();
    m_nodes[nodeId].id = childId;//m_nodes.size();
    ID northId = m_nodes[nodeId].neighbours[0];
    ID eastId  = m_nodes[nodeId].neighbours[1];
    ID southId = m_nodes[nodeId].neighbours[2];
    ID westId  = m_nodes[nodeId].neighbours[3];

    //update neighbour at neighbour id's children with this children if is children
    //what if children are parents too???
    //should I use dirVec???
    

    auto updateNeighbour = [this](ID neighbourId, ID thisId, QUADRANT neighbourQuadrant, DIR neighbourSide) -> ID
    {
        if(neighbourId == INVALID_ID)
            return INVALID_ID;
        if(m_nodes[neighbourId].isLeaf)
            return neighbourId;//no need to update as already goes to parent
        //update ne child's right neighbour is this guy... and all its right children TOO
        updateNeighbours(m_nodes[neighbourId].id+neighbourQuadrant, thisId, neighbourSide);
//        m_nodes[m_nodes[neighbourId].id+neighbourQuadrant].neighbours[neighbourSide] = thisId;
        return m_nodes[neighbourId].id+neighbourQuadrant;
    };
    
    //SW
    m_nodes.emplace_back(oldDataId,   m_nodes[nodeId].dirVec,                       m_nodes[nodeId].depth+1, true,
                         childId + QUADRANT::NW,//N
                         childId + QUADRANT::SE,//E
                         INVALID_ID,//updateNeighbour(southId, childId+0, QUADRANT::NW, DIR::NORTH),//S
                         INVALID_ID);//updateNeighbour(westId,  childId+0, QUADRANT::SE, DIR::EAST));//W
    //SE
    m_nodes.emplace_back(m_numLeaves, m_nodes[nodeId].dirVec | 1 << (28 - 2*depth), m_nodes[nodeId].depth+1, true,
                         childId + QUADRANT::NE,//N
                         INVALID_ID,//updateNeighbour(eastId,  childId+1, QUADRANT::SW, DIR::WEST),//E
                         INVALID_ID,//updateNeighbour(southId, childId+1, QUADRANT::NE, DIR::NORTH),//S
                         childId + QUADRANT::SW);//W
    ++m_numLeaves;
    //NW
    m_nodes.emplace_back(m_numLeaves, m_nodes[nodeId].dirVec | 2 << (28 - 2*depth), m_nodes[nodeId].depth+1, true,
                         INVALID_ID,//updateNeighbour(northId, childId+2, QUADRANT::SW, DIR::SOUTH),//N
                         childId + QUADRANT::NE,//E
                         childId + QUADRANT::SW,//S
                         INVALID_ID);//updateNeighbour(westId,  childId+2, QUADRANT::NE, DIR::EAST));//W
    ++m_numLeaves;
    //NE
    m_nodes.emplace_back(m_numLeaves, m_nodes[nodeId].dirVec | 3 << (28 - 2*depth), m_nodes[nodeId].depth+1, true,
                         INVALID_ID,//updateNeighbour(northId, childId+3, QUADRANT::SE, DIR::SOUTH),//N
                         INVALID_ID,//updateNeighbour(eastId,  childId+3, QUADRANT::NW, DIR::WEST),//E
                         childId + QUADRANT::SE,//S
                         childId + QUADRANT::NW);//W
    ++m_numLeaves;

//can't call till the children are made if want neighbour wrapping to work as updateNeighbour updates the children too...
    m_nodes[childId + QUADRANT::SW].neighbours[DIR::SOUTH] = updateNeighbour(southId, childId+0, QUADRANT::NW, DIR::NORTH);//S
    m_nodes[childId + QUADRANT::SW].neighbours[DIR::WEST]  = updateNeighbour(westId,  childId+0, QUADRANT::SE, DIR::EAST);//W
    m_nodes[childId + QUADRANT::SE].neighbours[DIR::EAST]  = updateNeighbour(eastId,  childId+1, QUADRANT::SW, DIR::WEST);//E
    m_nodes[childId + QUADRANT::SE].neighbours[DIR::SOUTH] = updateNeighbour(southId, childId+1, QUADRANT::NE, DIR::NORTH);//S
    m_nodes[childId + QUADRANT::NW].neighbours[DIR::NORTH] = updateNeighbour(northId, childId+2, QUADRANT::SW, DIR::SOUTH);//N
    m_nodes[childId + QUADRANT::NW].neighbours[DIR::WEST]  = updateNeighbour(westId,  childId+2, QUADRANT::NE, DIR::EAST);//W
    m_nodes[childId + QUADRANT::NE].neighbours[DIR::NORTH] = updateNeighbour(northId, childId+3, QUADRANT::SE, DIR::SOUTH);//N
    m_nodes[childId + QUADRANT::NE].neighbours[DIR::EAST]  = updateNeighbour(eastId,  childId+3, QUADRANT::NW, DIR::WEST);//E
}

//FASTER???????
//~75% of this QuadTree Algorithm's time is spent here
void QuadTree::balanceNeighbours(NodeId nodeId, DIR skipDir)
{
    int depth = m_nodes[nodeId].depth;
    ID dirVec = m_nodes[nodeId].dirVec;

    if(depth == 0) return;

    bool isEast  = dirVec & (1 << (30-2*depth));
    bool isNorth = dirVec & (2 << (30-2*depth));

    if(skipDir != DIR::NORTH && isNorth)
    {
//        ID northDirVec = neighbourDirVec(dirVec, depth,DIR::NORTH);
//#ifndef DQT_WRAP_NEIGHBOURS
//        if(northDirVec & OUT_OF_BOUNDS_MASK) goto NorthExit;
//#endif
        ID northNodeId = m_nodes[nodeId].neighbours[DIR::NORTH];//_atDir(northDirVec, depth-1);
        if(northNodeId != INVALID_ID && m_nodes[northNodeId].isLeaf && m_nodes[northNodeId].depth < depth)
        {
            subdivide(northNodeId);
            balanceNeighbours(northNodeId,DIR::SOUTH);
        }
    }
//    NorthExit:
    if(skipDir != DIR::EAST && isEast)
    {
//        ID eastDirVec = neighbourDirVec(dirVec, depth, DIR::EAST);
//#ifndef DQT_WRAP_NEIGHBOURS
//        if(eastDirVec & OUT_OF_BOUNDS_MASK) goto EastExit;
//#endif
        ID eastNodeId = m_nodes[nodeId].neighbours[DIR::EAST];//_atDir(eastDirVec, depth-1);
        if(eastNodeId != INVALID_ID && m_nodes[eastNodeId].isLeaf && m_nodes[eastNodeId].depth < depth)
        {
            subdivide(eastNodeId);
            balanceNeighbours(eastNodeId,DIR::WEST);
        }
    }
//    EastExit:
    if(skipDir != DIR::SOUTH && !isNorth)
    {
//        ID southDirVec = neighbourDirVec(dirVec, depth, DIR::SOUTH);
//#ifndef DQT_WRAP_NEIGHBOURS
//        if(southDirVec & OUT_OF_BOUNDS_MASK) goto SouthExit;
//#endif
        ID southNodeId = m_nodes[nodeId].neighbours[DIR::SOUTH];//_atDir(southDirVec, depth-1);
        if(southNodeId != INVALID_ID && m_nodes[southNodeId].isLeaf && m_nodes[southNodeId].depth < depth)
        {
            subdivide(southNodeId);
            balanceNeighbours(southNodeId,DIR::NORTH);
        }
    }
//    SouthExit:
    if(skipDir != DIR::WEST && !isEast)
    {
//        ID westDirVec = neighbourDirVec(dirVec, depth, DIR::WEST);
//#ifndef DQT_WRAP_NEIGHBOURS
//        if(westDirVec & OUT_OF_BOUNDS_MASK) return;
//#endif
        ID westNodeId = m_nodes[nodeId].neighbours[DIR::WEST];//_atDir(westDirVec, depth-1);
        if(westNodeId != INVALID_ID && m_nodes[westNodeId].isLeaf && m_nodes[westNodeId].depth < depth)
        {
            subdivide(westNodeId);
            balanceNeighbours(westNodeId,DIR::EAST);
        }
    }
}
*/