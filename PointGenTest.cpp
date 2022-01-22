#include <SFML/Graphics.hpp>
#include "QuadTreeTesselator.hpp"

const std::vector<Point> L1 = 
{
    {0,1},
    {2,3},
    {4,1}
};

const std::vector<Point> L2 = 
{
    {1,4},
    {1,2},
    {0,3}
};

const std::vector<Point> L3 = 
{
    {4,3},
    {3,2},
    {3,4}
};

int main()
{
    srand(time(NULL));
    int sz = 4;
    TESS::QuadTreeTesselator QTT(sz,sz,{0,0},1);
    QTT.addLine(L1,1);
    QTT.addLine(L2,1);
    QTT.addLine(L3,1);

    QTT.triangulate();

    const std::vector<Point> &points = QTT.getPoints();

    std::vector<sf::CircleShape> circles;
    sf::CircleShape circle;
    circle.setFillColor(sf::Color(255,0,0,128));
    circle.setOrigin(0.25f,0.25f);
    circle.setRadius(0.25f);

    for(auto &p : points)
    {
        circle.setPosition(p.x,p.y);
        circles.push_back(circle);
    }

    std::vector<sf::Color> colors;
    for(size_t i = 0; i < points.size(); ++i)
        colors.emplace_back(rand()%256,rand()%256,rand()%256);
    
    std::vector<sf::Vertex> triangles;
    for(size_t i = 0; i < QTT.getTriangles(1).size(); ++i)
    {
        TESS::ID I = QTT.getTriangles(1)[i];
        triangles.emplace_back(sf::Vector2f(points[I].x,points[I].y),colors[I]);
    }

    for(size_t i = 0; i < QTT.getTriangles(TESS::EMPTY_SPACE_ID).size(); ++i)
    {
        TESS::ID I = QTT.getTriangles(TESS::EMPTY_SPACE_ID)[i];
        triangles.emplace_back(sf::Vector2f(points[I].x,points[I].y),colors[I]);
    }

    ////////////
    sf::RenderWindow window(sf::VideoMode(960,960),"GridPts");//sf::Style::Fullscreen);//800,800),"GridPts");
    sf::View view;
    view.setCenter(sz/2,sz/2);
    view.setSize(static_cast<float>(sz)*1.1f,static_cast<float>(sz)*-1.1f);
    window.setView(view);

    sf::Event event;
    while (window.isOpen())
    {
        window.clear(sf::Color(32,32,32));
        window.draw(triangles.data(),triangles.size(),sf::Triangles);
//        for(auto &c : circles)
//            window.draw(c);//see any brighter spots -> duplicate points
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