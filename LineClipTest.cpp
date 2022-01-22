#include "Vec2.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>

//Just for reference really here.

//modified from https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
//(0-w)X(0-h) box
bool clipLine(const Point &a, const Point &b, const float w, const float h, Point &entryPt, Point &exitPt)
{
    typedef int OutCode;
    const int INSIDE = 0;
    const int LEFT = 1;
    const int RIGHT = 2;
    const int BOTTOM = 4;
    const int TOP = 8;

    auto computeOutCode = [](const Point &p, const float w, const float h)->OutCode
    {
        OutCode code = INSIDE;

        if(p.x < 0)
            code |= LEFT;
        else if(p.x > w)
            code |= RIGHT;
        if(p.y < 0)
            code |= BOTTOM;
        else if(p.y > h)
            code |= TOP;

        return code;
    };

    entryPt = a;
    exitPt = b;
    
    OutCode outCodeA = computeOutCode(entryPt,w,h);
    OutCode outCodeB = computeOutCode(exitPt,w,h);
    
    while (true)
    {
        if(!(outCodeA | outCodeB))
            return true;//both points inside !(00|00)
        if (outCodeA & outCodeB)
            return false;//both points share exclusion zone... so no way can go in
        
        OutCode theOutsideOutcode = outCodeA > outCodeB ? outCodeA : outCodeB;//so if one is inside (00) choose the outside outcode

        float x,y;
        //find where clips rectangle lines .. clip to line... repeats loop if and only if both endpoints lay outside rectangle
        if(theOutsideOutcode & TOP)
        {
            x = a.x + (b.x - a.x) * (h - a.y) / (b.y - a.y);
            y = h;
        }
        else if(theOutsideOutcode & BOTTOM)
        {
            x = a.x + (b.x - a.x) * (0 - a.y) / (b.y - a.y);
            y = 0;
        }
        else if(theOutsideOutcode & RIGHT)
        {
            y = a.y + (b.y - a.y) * (w - a.x) / (b.x - a.x);//w
            x = w;
        }
        else// if(theOutsideOutcode & LEFT)
        {
            y = a.y + (b.y - a.y) * (0 - a.x) / (b.x - a.x);
            x = 0;
        }

        if(theOutsideOutcode == outCodeA)
        {
            entryPt.x = x;
            entryPt.y = y;
            outCodeA = computeOutCode(entryPt,w,h);
        }
        else
        {
            exitPt.x = x;
            exitPt.y = y;
            outCodeB = computeOutCode(exitPt,w,h);
        }
    }   
}

bool collide(const Point &a, const Point &b, const float r = .5f)
{
    Point dv = b - a;
    return (dv.x * dv.x + dv.y * dv.y) < (r * r);
}

int main()
{
    srand(time(NULL));
    Point boxSz(10,10);
    sf::RectangleShape boxShape;
    boxShape.setPosition(0.f,0.f);
    boxShape.setSize(boxSz);
    boxShape.setFillColor(sf::Color(255,255,0,128));

    Point a(rand()%10,rand()%10);
    Point b(rand()%10,rand()%10);

    sf::Vertex line[6] = 
    {
        {a,sf::Color::Red},
        {a,sf::Color::Red},
        {a,sf::Color::Green},
        {b,sf::Color::Green},
        {b,sf::Color::Red},
        {b,sf::Color::Red}
    };

    sf::CircleShape handleShape;
    handleShape.setFillColor(sf::Color(0,0,255,64));
    handleShape.setOrigin(0.5f,0.5f);
    handleShape.setRadius(0.5f);

    sf::RenderWindow window(sf::VideoMode(800,800),"Line Clipping");
    sf::View view;
    view.setCenter(5,5);
    view.setSize(20,20);
    window.setView(view);

    bool drawHandler = false;
    bool hoverA = false;
    bool hoverB = false;
    bool amMovingA = false;
    bool amMovingB = false;
    while (window.isOpen())
    {
        window.clear(sf::Color::Black);
        window.draw(boxShape);
        window.draw(line,6,sf::Lines);
        if(drawHandler)
            window.draw(handleShape);
        window.display();

        sf::Event event;
        window.waitEvent(event);
        if(event.type == sf::Event::Closed || event.type == sf::Event::KeyPressed)
            window.close();
        if(event.type == sf::Event::MouseMoved)
        {
            sf::Vector2i mouseCoords(event.mouseMove.x,event.mouseMove.y);
            auto mousePos = window.mapPixelToCoords(mouseCoords);
            hoverA = collide(mousePos,a);
            hoverB = collide(mousePos,b);
            drawHandler = hoverA | hoverB;
            if(hoverA)
                handleShape.setPosition(a);
            if(hoverB)
                handleShape.setPosition(b);

            if(amMovingA) a = mousePos;
            if(amMovingB) b = mousePos;
            if(amMovingA || amMovingB)
            {
                Point entryPt, exitPt;
                if(clipLine(a,b,boxSz.x,boxSz.y,entryPt,exitPt))
                {
                    line[1].position = line[2].position = entryPt;
                    line[3].position = line[4].position = exitPt;
                }
                else
                {
                    line[1].position = line[2].position = a;
                    line[3].position = line[4].position = a;
                }
                line[0].position = a;
                line[5].position = b;
            }
        }
        if(event.type == sf::Event::MouseButtonPressed)
        {//states....
            if(hoverA)
                amMovingA = true;
            else if(hoverB)
                amMovingB = true;
        }
        if(event.type == sf::Event::MouseButtonReleased)
        {
            amMovingA = amMovingB = false;
        }
    }
    
}