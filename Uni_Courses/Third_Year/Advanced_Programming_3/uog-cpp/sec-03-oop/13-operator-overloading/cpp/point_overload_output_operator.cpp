#include <iostream>

class Point
{
private:
    double x{};
    double y{};
    double z{};

public:
    Point(double x=0.0, double y=0.0, double z=0.0)
      : x{x}, y{y}, z{z} { }

    friend std::ostream& operator<< (std::ostream& out, const Point& point);
};

std::ostream& operator<< (std::ostream& out, const Point& point)
{
    // Since operator<< is a friend of the Point class, 
    // we can access Point's members directly.
    out << "Point(" << point.x << ", " 
        << point.y << ", " << point.z << ')'; // actual output done here

    return out; // return std::ostream so we can chain calls to operator<<
}

int main()
{
    const Point point1 { 42.0, 24.0, 12.0 };
    std::cout << point1 << '\n';
    
    
}