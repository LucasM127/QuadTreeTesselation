#include "TriangulateCellNode.hpp"
#include "Quadrants.hpp"

#include <iostream>

std::ostream &operator<<(std::ostream &os, const Point &p)
{
    return os << p.x << " " << p.y;
}

void test(TESS::CellInfo &C)
{
    TESS::triangulateCellNode(C);

    std::cout<<"Line polygons\n";
    for(size_t i = 0; i < C.linePolygonIds.size(); i += 2)// auto i : C.linePolygonIds)
        std::cout<<"line "<<i/2<<":(r)"<<C.linePolygonIds[i]<<" (l)"<<C.linePolygonIds[i+1]<<"\n";
    std::cout<<std::endl;

    std::cout<<"Triangle polygon ids...\n";
    for(auto i : C.triangleIds)
        std::cout<<i<<" ";
    std::cout<<std::endl;

    for(size_t i = 0; i < C.triangles.size(); i+=3)//auto i : C.triangles)
        std::cout<<C.triangles[i]<<"-"<<C.triangles[i+1]<<"-"<<C.triangles[i+2]<<"\n";
    std::cout<<std::endl;

    std::cout<<"Up Neighbour: "<<C.neighbourIds[TESS::DIR::UP]<<"\n";
    std::cout<<"Down Neighbour: "<<C.neighbourIds[TESS::DIR::DOWN]<<"\n";
    std::cout<<"Left Neighbour: "<<C.neighbourIds[TESS::DIR::LEFT]<<"\n";
    std::cout<<"Right Neighbour: "<<C.neighbourIds[TESS::DIR::RIGHT]<<"\n";
}

void diamondTest()
{//Good
    TESS::CellInfo C;
    C.cornerIds[TESS::QUADRANT::NW] = 10;
    C.cornerIds[TESS::QUADRANT::SW] = 11;
    C.cornerIds[TESS::QUADRANT::SE] = 12;
    C.cornerIds[TESS::QUADRANT::NE] = 13;

    //Diamond Shape Case
    C.lines = 
    {
        {0.0f,0.5f},
        {0.5f,1.0f},
        {0.5f,1.0f},
        {1.0f,0.5f},
        {1.0f,0.5f},
        {0.5f,0.0f},
        {0.5f,0.0f},
        {0.0f,0.5f} 
    };//this is the same...

    C.linePolygonIds = {1,0,1,0,1,0,1,0};
    C.linePointIds = {0,1,2,3,4,5,6,7};

    test(C);
}

void test2()//in sceneTest not showing the triangle properly
{
    TESS::CellInfo C;
    C.cornerIds[TESS::QUADRANT::NW] = 10;
    C.cornerIds[TESS::QUADRANT::SW] = 11;
    C.cornerIds[TESS::QUADRANT::SE] = 12;
    C.cornerIds[TESS::QUADRANT::NE] = 13;

    C.steinerIds[TESS::DIR::UP] = 20;
    C.steinerIds[TESS::DIR::LEFT] = 21;
//    C.steinerIds[TESS::DIR::DOWN] = 22;
    C.steinerIds[TESS::DIR::RIGHT] = 23;

    C.lines = 
    {
        {0.0f,0.0f},
        {0.0f,0.5f}
    };//this is the same...

    C.linePolygonIds = {0,5};
    C.linePointIds = {0,1};

    test(C);
}


void test3()//from scenetest
{
    TESS::CellInfo C;
    C.cornerIds[TESS::QUADRANT::NW] = 10;
    C.cornerIds[TESS::QUADRANT::SW] = 11;
    C.cornerIds[TESS::QUADRANT::SE] = 12;
    C.cornerIds[TESS::QUADRANT::NE] = 13;

//    C.steinerIds[TESS::DIR::UP] = 20;
//    C.steinerIds[TESS::DIR::LEFT] = 21;
    C.steinerIds[TESS::DIR::DOWN] = 22;
    C.steinerIds[TESS::DIR::RIGHT] = 23;

    C.lines = 
    {
        {0.5f,0.0f},
        {1.0f,0.5f}
    };

    C.linePolygonIds = {0,5};
    C.linePointIds = {0,1};

    test(C);
}

void test4()//from scenetest
{
    TESS::CellInfo C;
    C.cornerIds[TESS::QUADRANT::NW] = 10;
    C.cornerIds[TESS::QUADRANT::SW] = 11;
    C.cornerIds[TESS::QUADRANT::SE] = 12;
    C.cornerIds[TESS::QUADRANT::NE] = 13;

//    C.steinerIds[TESS::DIR::UP] = 20;
//    C.steinerIds[TESS::DIR::LEFT] = 21;
    C.steinerIds[TESS::DIR::DOWN] = 22;
    C.steinerIds[TESS::DIR::RIGHT] = 23;

    C.lines = 
    {
        {0.0f,1.0f},
        {0.5f,0.0f},
        {0.0f,1.0f},
        {1.0f,0.5f}
    };

    C.linePolygonIds = {0,5,5,0};
    C.linePointIds = {0,1,2,3};

    test(C);
}

int main()
{
    test4();
//    diamondTest();
    /*

    C.lines = 
    {
        {0.0f,0.0f},
        {1.0f,1.0f}
    };
    C.lineIds = {1};
    C.linePointIds = {0,1};
*/
/*
    //range walk test want 5-4-3-0-3-1-3-2-4-5 // ignore the empty range 3
    C.lines = 
    {
        {0.00f,0.75f},
        {0.25f,1.00f},
        {0.75f,1.00f},
        {1.00f,0.75f},
        {0.50f,1.00f},
        {1.00f,0.50f},
        {1.00f,0.50f},
        {0.75f,0.00f},
        {0.25f,0.00f},
        {0.00f,0.25f}
    };
    C.lineIds = {1,2,3,4,5};
    C.linePointIds = {0,1,2,3,4,5,6,7,8,9};
*/
/*
    //and an Octagon case for giggles
    C.lines = 
    {
        {0.00f,0.75f},
        {0.25f,1.00f},
        {0.75f,1.00f},
        {1.00f,0.75f},
        {1.00f,0.25f},
        {0.75f,0.00f},
        {0.25f,0.00f},
        {0.00f,0.25f}
    };
    C.lineIds = {1,2,3,4};
    C.linePointIds = {0,1,2,3,4,5,6,7};
*/

/*
// Case 1
    C.lines = 
    {
        {0.0f,0.5f},
        {0.5f,1.0f},
        {0.5f,1.0f},
        {0.0f,0.0f},
        {0.0f,0.0f},
        {1.0f,0.5f}
    };

    C.lineIds = {1,1,1};
    C.linePointIds = {0,1,2,3,4,5};
*/
/*
    C.lines = 
    {
        {0.0f,0.0f},
        {0.0f,1.0f},
        {0.5f,1.0f},
        {1.0f,0.0f}
    };

    C.lineIds = {1,1};
    C.linePointIds = {0,1,2,3};
*/
    

    return 0;
}