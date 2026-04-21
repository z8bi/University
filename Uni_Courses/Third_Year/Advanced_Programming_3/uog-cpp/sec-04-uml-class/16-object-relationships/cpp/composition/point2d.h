#ifndef POINT2D_H
#define POINT2D_H

#include <iostream>

class Point2D
{
private:
    double x;
    double y;

public:
    // A default constructor
    Point2D()
        : x{ 0 }, y{ 0 }
    {
    }

    // A specific constructor
    Point2D(double x, double y)
        : x{ x }, y{ y }
    {
    }

    // An overloaded output operator
    friend std::ostream& operator<<(std::ostream& out, const Point2D& point)
    {
        out << '(' << point.x << ", " << point.y << ')';
        return out;
    }

    // Access functions
    void set_point(double x, double y)
    {
        this->x = x;
        this->y = y;
    }

};

#endif