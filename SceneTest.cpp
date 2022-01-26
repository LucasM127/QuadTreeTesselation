#include <SFML/Graphics.hpp>
#include "QuadTreeTesselator.hpp"

const std::vector<Point> LeftWallPoints = 
{
    { 9,16},
    { 5,14},
    { 1,10},
    { 1, 6},
    { 3, 0}
};

const std::vector<Point> RightWallPoints = 
{
    {29, 0},
    {31, 6},
    {31,10},
    {27,14},
    {23,16}
};

const std::vector<Point> LeftAltarPoints =
{
    { 4, 0},
    { 6, 1},
    { 7, 2},
    { 5, 4},
    { 8, 6},
    { 7, 4},
    {11, 4},
    {10, 6},
    {13, 4},
    {11, 2},
    {12, 1},
    {14, 0}
};

// = L2L + 14...
const std::vector<Point> RightAltarPoints =
{
    {18, 0},
    {20, 1},
    {21, 2},
    {19, 4},
    {22, 6},
    {21, 4},
    {25, 4},
    {24, 6},
    {27, 4},
    {25, 2},
    {26, 1},
    {28, 0}
};

//looping
const std::vector<Point> LeftFlamePoints =
{
    { 9, 6},
    { 8, 8},
    { 8,10},
    { 9, 8},
    {10,10},
    {10, 8},
    { 9, 6}
};

//looping
const std::vector<Point> RightFlamePoints =
{
    {23,10},
    {22, 8},
    {22,10},
    {23,12},
    {24,10},
    {24, 8},
    {23,10}
};

const std::vector<Point> OuterRootPoints =
{
    {21,16},
    {21,14},
    {19,13},
    {18,10},
    {18, 9},
    {20, 7},
    {19, 5},
    {17, 4},
    {15, 4},
    {13, 5},
    {12, 7},
    {14, 9},
    {14,10},
    {13,13},
    {11,14},
    {11,16}
};

const std::vector<Point> InnerRootPoints = 
{
    {13,16},
    {13,15},
    {15,13},
    {17,12},
    {15,11},
    {15, 9},
    {18, 7},
    {17, 6},
    {15, 6},
    {14, 7},
    {17, 7},
    {15, 8},
    {13, 7},
    {15, 5},
    {17, 5},
    {19, 7},
    {17, 9},
    {15,10},
    {17,11},
    {17,13},
    {19,15},
    {19,16}
};

static inline Point intYatXPoint(const Point &a, const Point &b, float x)
{
    float y = a.y + (x - a.x) * (b.y - a.y) / (b.x - a.x);
    return Point(x,y);
}

//Just a way to clip the line so not go out of bounds
void addClippedLine(const float x, const std::vector<Point> &inPoints, 
            TESS::QuadTreeTesselator &leftQTT, TESS::QuadTreeTesselator &rightQTT,
            const unsigned int rightPolygonId, const unsigned int leftPolygonId = 0)
{
    if(inPoints.size()==0)
        return;
    
    TESS::QuadTreeTesselator *pQTT;
    std::vector<Point> line;
    if(inPoints.front().x < x)
        pQTT = &leftQTT;
    else
        pQTT = &rightQTT;
    
    line.push_back(inPoints.front());

    for(size_t i = 1; i < inPoints.size(); ++i)
    {
        const Point &a = inPoints[i-1];
        const Point &b = inPoints[i];

        const bool aLeft = a.x < x;
        const bool bLeft = b.x < x;

        //if a and b on the same side just push the point back
        if(aLeft == bLeft)
        {
            line.push_back(b);
            continue;
        }
        
        Point p = intYatXPoint(a,b,x);
        line.push_back(p);//instead of b
        pQTT->addLine(line,rightPolygonId,leftPolygonId);
        
        if(aLeft)
            pQTT = &rightQTT;
        else
            pQTT = &leftQTT;
        line.clear();

        line.push_back(p);
        line.push_back(b);
    }
    pQTT->addLine(line,rightPolygonId,leftPolygonId);
}

std::vector<int> S;//Smoothing Effect
std::vector<int> C;//Grid Effect
std::vector<int> T;//Triangle Effect

void drawRegion(const TESS::QuadTreeTesselator &QTT, const unsigned int id, const sf::Color baseColor, std::vector<sf::Vertex> &triangles)
{
    auto &triangleIds = QTT.getTriangles(id);
    auto &points = QTT.getPoints();
    auto &cellIds = QTT.getCellTriIds(id);
    for(size_t k = 0; k < triangleIds.size(); k++)
    {
        unsigned int i = triangleIds[k];
        int t = T[k%T.size()];
        int s = S[i%S.size()];
        int c = C[cellIds[(k/3)]%C.size()];
        sf::Color color = baseColor;
        color.r += t + s + c;
        color.g += t + s + c;
        color.b += t + s + c;
        triangles.emplace_back(points[i],color);
    }
}

int main()
{
    srand(time(NULL));

    for(int i = 0; i < 16; i++)
        S.emplace_back(rand()%16);
    for(int i = 0; i < 16; i++)
        T.emplace_back(rand()%16);
    for(int i = 0; i < 16; i++)
        C.emplace_back(rand()%32);

    int sz = 16;
    //2 16 x 16 QuadTrees with second one offset by 16 step size of 1
    TESS::QuadTreeTesselator QTT(sz,sz,{0,0},1);//32 x 16!!!
    TESS::QuadTreeTesselator QTT2(sz,sz,{16.f,0},1);

    enum Regions : int
    {
        Background = 0,
        Wall,
        Altar,
        LeftFlame,
        RightFlame,
        Root,
        RootWater,
        NumRegions
    };
    
    QTT.addLine(LeftWallPoints,Regions::Wall);
    QTT.addLine(LeftAltarPoints,Regions::Altar);
    QTT.addLine(LeftFlamePoints,Regions::LeftFlame);
    addClippedLine(sz,OuterRootPoints,QTT,QTT2,Regions::Root);//,Regions::Root);
    addClippedLine(sz,InnerRootPoints,QTT,QTT2,Regions::Root,Regions::RootWater);
    QTT2.addLine(RightFlamePoints,Regions::RightFlame);
    QTT2.addLine(RightAltarPoints,Regions::Altar);
    QTT2.addLine(RightWallPoints,Regions::Wall);
    
    QTT.triangulate();
    QTT2.triangulate();

    std::unordered_map<int,sf::Color> colorMap;
    colorMap[Regions::Background] = sf::Color::Black;
    colorMap[Regions::Wall] = sf::Color(32,0,64);
    colorMap[Regions::Altar] = sf::Color(192,0,0);
    colorMap[Regions::LeftFlame] = sf::Color(0,192,0);
    colorMap[Regions::RightFlame] = sf::Color(0,0,128);
    colorMap[Regions::Root] = sf::Color(64,32,16);
    colorMap[Regions::RootWater] = sf::Color(0,128,128);
    
    std::vector<sf::Vertex> triangles;
    for(int i = 0; i < Regions::NumRegions; i++)
    {
        drawRegion(QTT,i,colorMap[i],triangles);
        drawRegion(QTT2,i,colorMap[i],triangles);
    }

    std::vector<sf::CircleShape> circles;
    sf::CircleShape circle;
    circle.setFillColor(sf::Color(255,0,0,128));
    circle.setOrigin(0.25f,0.25f);
    circle.setRadius(0.25f);

    for(auto &p : QTT.getPoints())
    {
        circle.setPosition(p.x,p.y);
        circles.push_back(circle);
    }

    sf::RenderWindow window(sf::VideoMode(1280,640),"GridPts");//sf::Style::Fullscreen);//800,800),"GridPts");
    sf::View view;
    view.setCenter(sz,sz/2);
    view.setSize(static_cast<float>(2*sz)+1.f,static_cast<float>(sz)*-1.f - 1.f);
    window.setView(view);

    sf::Event event;
    while (window.isOpen())
    {
        window.clear(sf::Color(32,32,32));
        window.draw(triangles.data(),triangles.size(),sf::Triangles);
        //for(auto &c : circles)
        //    window.draw(c);//see any brighter spots -> duplicate points
        window.display();

        window.waitEvent(event);
        if (event.type == sf::Event::Closed)
        {
            window.close();
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            window.close();
    }

    return 0;
}