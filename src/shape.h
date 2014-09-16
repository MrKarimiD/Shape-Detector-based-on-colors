#ifndef SHAPE_H
#define SHAPE_H

#include <vector>
#include <iostream>

using namespace std;

class Shape
{
public:
    explicit Shape();
    void set(float x, float y,double roundedRadios,std::string color,std::string type);
    float position_x;
    float position_y;
    double roundedRadios;
    std::string color;
    std::string type;
};

#endif // SHAPE_H
