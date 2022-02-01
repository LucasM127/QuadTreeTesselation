//NeighbourFinding
#include <SFML/Graphics.hpp>
#include "DQT.hpp"

struct Point
{
    int x,y;
};

void drawNode(TESS::QuadTree &tree, const TESS::Node &node, std::vector<sf::Vertex> &shape)
{
    int v = rand()%128;
    sf::Color color = sf::Color(v,v,v,128);

    TESS::Box n = tree.calcBounds(node);

    sf::Vector2f a(n.x,n.y);
    sf::Vector2f b(n.x,n.y+n.sz);
    sf::Vector2f c(n.x+n.sz,n.y+n.sz);
    sf::Vector2f d(n.x+n.sz,n.y);

    shape.emplace_back(a,color);
    shape.emplace_back(b,color);
    shape.emplace_back(b,color);
    shape.emplace_back(c,color);
    shape.emplace_back(c,color);
    shape.emplace_back(d,color);
    shape.emplace_back(d,color);
    shape.emplace_back(a,color);
}

void drawTree(TESS::QuadTree &tree, const TESS::Node &node, std::vector<sf::Vertex> &shape)
{
    if(!node.isLeaf)
    {
        drawTree(tree,tree.sw(node),shape);
        drawTree(tree,tree.se(node),shape);
        drawTree(tree,tree.nw(node),shape);
        drawTree(tree,tree.ne(node),shape);
        return;
    }

    drawNode(tree,node,shape);
}

void drawNodeTris(TESS::QuadTree &tree, const TESS::Node &node, std::vector<sf::Vertex> &shape, sf::Color color)
{
    TESS::Box n = tree.calcBounds(node);

    sf::Vector2f a(n.x,n.y);
    sf::Vector2f b(n.x,n.y+n.sz);
    sf::Vector2f c(n.x+n.sz,n.y+n.sz);
    sf::Vector2f d(n.x+n.sz,n.y);

    shape.emplace_back(a,color);
    shape.emplace_back(b,color);
    shape.emplace_back(c,color);
    shape.emplace_back(c,color);
    shape.emplace_back(d,color);
    shape.emplace_back(a,color);
}

void drawNeighbours(TESS::QuadTree &tree, const TESS::Node &node, std::vector<sf::Vertex> &shape)
{
    const sf::Color northColor = sf::Color(192,64,64,192);
    const sf::Color eastColor  = sf::Color(64,64,192,192);
    const sf::Color southColor = sf::Color(64,192,64,192);
    const sf::Color westColor  = sf::Color(192,192,64,192);

    shape.clear();

    if(node.id == TESS::INVALID_ID)
        return;
    
    const TESS::Node &northNode = tree.neighbour(node,TESS::DIR::NORTH);
    const TESS::Node &eastNode = tree.neighbour(node,TESS::DIR::EAST);
    const TESS::Node &southNode = tree.neighbour(node,TESS::DIR::SOUTH);
    const TESS::Node &westNode = tree.neighbour(node,TESS::DIR::WEST);
    if(northNode.id != TESS::INVALID_ID)
        drawNodeTris(tree,northNode,shape,northColor);
    if(eastNode.id != TESS::INVALID_ID)
        drawNodeTris(tree,eastNode,shape,eastColor);
    if(southNode.id != TESS::INVALID_ID)
        drawNodeTris(tree,southNode,shape,southColor);
    if(westNode.id != TESS::INVALID_ID)
        drawNodeTris(tree,westNode,shape,westColor);
}

int main()
{
    srand(0);//time(nullptr));
    int sz = 16;
    int n = 25;
    TESS::QuadTree quadTree(0,sz,0,sz);

    std::vector<Point> points;
    /*{
        {7,7},
        {14,5},
        {15,15}
    };*/
    for(int i = 0; i < n; ++i)
        points.push_back({rand()%sz,rand()%sz});

//    for(int i = 0; i < 2; ++i)
//    quadTree.subdivide(points[i].x,points[i].y);
    for(auto &p : points)
        quadTree.subdivide(p.x,p.y);

    std::vector<sf::Vertex> treeShape;
    std::vector<sf::Vertex> neighbourShape;
    drawTree(quadTree,quadTree.root(),treeShape);

    sf::RenderWindow window(sf::VideoMode(960,960),"QuadTree");
    sf::View view;
    float viewSz = static_cast<float>(sz) * 1.1f;
    view.setCenter(sz/2,sz/2);
    view.setSize(viewSz,-viewSz);
    window.setView(view);

    sf::Color bgColor = sf::Color(200,220,226);
    float bgSz = sz;
    sf::Vertex bgShape[6] = 
    {
        {{0,0},bgColor},
        {{0,bgSz},bgColor},
        {{bgSz,0},bgColor},
        {{bgSz,0},bgColor},
        {{0,bgSz},bgColor},
        {{bgSz,bgSz},bgColor}
    };

    TESS::NodeId mouseNodeId;

    while (window.isOpen())
    {
        window.clear(sf::Color::White);
        window.draw(bgShape,6,sf::Triangles);
        window.draw(neighbourShape.data(),neighbourShape.size(),sf::Triangles);
        window.draw(treeShape.data(),treeShape.size(),sf::Lines);
        window.display();

        sf::Event event;
        window.waitEvent(event);
        switch (event.type)
        {
        case sf::Event::Closed:
        case sf::Event::KeyPressed:
            window.close();
            break;
        case sf::Event::MouseMoved:
        {
            sf::Vector2i mouseCoord(event.mouseMove.x,event.mouseMove.y);
            sf::Vector2f mousePos = window.mapPixelToCoords(mouseCoord);
            const TESS::Node &node = quadTree.at(mousePos.x,mousePos.y);
            if(node.id != mouseNodeId)
            {
                mouseNodeId = node.id;
                drawNeighbours(quadTree,node,neighbourShape);
            }
        }
        
        default:
            break;
        }
    }

    return 0;
}