#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <cmath>
#include "QuadTreeTesselator.hpp"
#include <iostream>
#include <random>
#include "SFMLCamera.hpp"


#include <chrono>
class Timer
{
public:
    Timer(const std::string &s) : string(s), start(std::chrono::high_resolution_clock::now()) {}
    ~Timer()
    {
        finish = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(finish-start);
        std::cout<<string<<" took "<<duration.count()<<" microseconds\n";
    }
    const std::string &string;
    std::chrono::time_point<std::chrono::high_resolution_clock> start, finish;
};

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
        color.a = 192;//64;
        triangles.emplace_back(points[i],color);
    }
}

//living on the edge
void debugDraw(const TESS::QuadTree &QT, const std::vector<TESS::CellInfo> &cellInfos, const std::vector<const TESS::Node*> &lineNodes, std::vector<sf::Vertex> &tris)
{//0 or 1
    sf::Color color;

    auto getColor = [](unsigned int id)->sf::Color
    {
        switch (id)
        {
        case -1:
            return sf::Color(255,0,0,128);
        case 0:
            return sf::Color(0,255,0,128);
        case 1:
            return sf::Color(0,0,255,128);
        default:
            return sf::Color::Black;
        }
    };

    //the dependencies are a bit wonky too
    //if neighbour draw green, else draw red line
    for(auto pnode : lineNodes)
    {
        auto C = cellInfos[pnode->id];
        TESS::Box n = QT.calcBounds(*pnode);
        float o = n.sz / 16.f;
        Point sw_in(n.x+o,n.y+o);
        Point se_in(n.x+n.sz-o,n.y+o);
        Point nw_in(n.x+o,n.y+n.sz-o);
        Point ne_in(n.x+n.sz-o,n.y+n.sz-o);
        Point sw(n.x,n.y);
        Point se(n.x+n.sz,n.y);
        Point nw(n.x,n.y+n.sz);
        Point ne(n.x+n.sz,n.y+n.sz);

        color = getColor(C.neighbourIds[TESS::DIR::NORTH]);
        {
            tris.emplace_back(nw,color);
            tris.emplace_back(ne,color);
            tris.emplace_back(nw_in,color);

            tris.emplace_back(nw_in,color);
            tris.emplace_back(ne,color);
            tris.emplace_back(ne_in,color);
        }
        color = getColor(C.neighbourIds[TESS::DIR::WEST]);
        {
            tris.emplace_back(nw,color);
            tris.emplace_back(sw,color);
            tris.emplace_back(nw_in,color);

            tris.emplace_back(nw_in,color);
            tris.emplace_back(sw,color);
            tris.emplace_back(sw_in,color);
        }
        color = getColor(C.neighbourIds[TESS::DIR::SOUTH]);
        {
            tris.emplace_back(se,color);
            tris.emplace_back(sw,color);
            tris.emplace_back(se_in,color);

            tris.emplace_back(se_in,color);
            tris.emplace_back(sw,color);
            tris.emplace_back(sw_in,color);
        }
        color = getColor(C.neighbourIds[TESS::DIR::EAST]);
        {
            tris.emplace_back(se,color);
            tris.emplace_back(ne,color);
            tris.emplace_back(se_in,color);

            tris.emplace_back(se_in,color);
            tris.emplace_back(ne,color);
            tris.emplace_back(ne_in,color);
        }
    }
}

int main()
{//should i put in my camera system again ? YEAH I NEEDS IT
    std::random_device rd;
    int seed = rd();
    //seed = 1638318210;//super faily seed for testing purposes
    //seed = -192464786;
    //seed = 2005654623;//border
    //seed = 1466811458;//normalized fail
    //seed = 273982980;//double free???
    std::cout<<seed<<std::endl;
    int sz = 4096;//64;

    std::mt19937 random_engine(seed);
    std::uniform_real_distribution<float> randFloat(0.f,1.f);

    std::vector<Point> pointyPolygon;//try a rectangle???
    for(float i = 0.f; i < 360.f; i += 1.f)//0.25f)//1.f)// 5.f)
    {
        float r = randFloat(random_engine) * (sz/32.f) + (3*sz/8);//< + (sz/8);
        float theta = i * M_PI / 180.f;
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        x = (int)x + sz/2;
        y = (int)y + sz/2;
        pointyPolygon.emplace_back(x,y);
    }
    pointyPolygon.emplace_back(pointyPolygon.front());

    for(int i = 0; i < 16; i++)
        S.emplace_back(rand()%32);
    for(int i = 0; i < 16; i++)
        T.emplace_back(rand()%64);//16);
    for(int i = 0; i < 16; i++)
        C.emplace_back(rand()%32);


//    Timer T("PointyMake");
    TESS::QuadTreeTesselator QTT(sz,sz,{0,0},1);
    
    QTT.addLine(pointyPolygon,1);
    
    QTT.triangulate();

    std::cout<<QTT.getPoints().size()<<" points generated\n";
    std::cout<<QTT.getCellTriIds(0).size()<<" & "<<QTT.getCellTriIds(1).size()<<" cells generated\n";
    std::cout<<QTT.getTriangles(0).size()<<" & "<<QTT.getTriangles(1).size()<<" triangle points generated\n";

    sf::Color polyColor = sf::Color(128,0,0);
    sf::Color bgColor = sf::Color(0,0,0);
    
    std::vector<sf::Vertex> triangles;
    std::vector<sf::Vertex> lineShape;
    std::vector<sf::Vertex> debugLines;

    debugDraw(QTT.getQT(),QTT.getCellInfos(),QTT.getLineNodes(),debugLines);
    
    drawRegion(QTT,0,bgColor,triangles);
    drawRegion(QTT,1,polyColor,triangles);

    for(auto &p : pointyPolygon)
        lineShape.emplace_back(p,sf::Color::Black);

    sf::RenderWindow window(sf::VideoMode(960,960),"GridPts");//sf::Style::Fullscreen);//800,800),"GridPts");
    sf::View view;
    view.setCenter(sz/2,sz/2);
    view.setSize(static_cast<float>(sz)+1.f,static_cast<float>(sz)*-1.f - 1.f);
    window.setView(view);

//    glLineWidth(4);

    Camera camera(window.getSize(),view);

    sf::Event event;
    while (window.isOpen())
    {
        window.setView(camera.getView());
        window.clear(sf::Color(224,224,224));//sf::Color(32,32,32));
        window.draw(triangles.data(),triangles.size(),sf::Triangles);
        window.draw(lineShape.data(),lineShape.size(),sf::LineStrip);
        window.draw(debugLines.data(),debugLines.size(),sf::Triangles);
        window.display();

        window.waitEvent(event);
        if (event.type == sf::Event::Closed)
        {
            window.close();
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            window.close();
        camera.handleEvent(event);
    }

    return 0;
}