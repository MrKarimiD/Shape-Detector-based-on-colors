#include "shape.h"

Shape::Shape()
{

}

void Shape::set(float x, float y, double roundedRadios, std::string color, std::string type)
{
    this->position_x = x;
    this->position_y = y;
    this->roundedRadios=roundedRadios;
    this->color=color;
    this->type=type;
}
