#include <iostream>

class Point
{
private:
    double x{};
    double y{};
    double z{};

public:
    Point(double x=0.0, double y=0.0, double z=0.0): x{x}, y{y}, z{z}
    {
    }

    friend std::ostream& operator<< (std::ostream& out, const Point& point); // Point has a dependency on std::ostream here
};

std::ostream& operator<< (std::ostream& out, const Point& point)
{
    // Since operator<< is a friend of the Point class, we can access Point's members directly.
    out << "Point(" << point.x << ", " << point.y << ", " << point.z << ')';

    return out;
}

int main()
{
    Point p { 42.0, 24.0, 42.24 };

    std::cout << p; // the program has a dependency on std::cout here

    return 0;
}