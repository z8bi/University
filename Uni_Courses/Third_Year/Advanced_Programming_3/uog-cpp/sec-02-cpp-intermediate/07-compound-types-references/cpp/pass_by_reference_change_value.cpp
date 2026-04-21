#include <iostream>

void add_value(double &y, double v) // y is bound to the actual object x
{
    y += v; // this modifies the actual object x
}

int main()
{
    double x { 42.0 };
    std::cout << "value of x = " << x << '\n';
    add_value(x, 24.0);
    std::cout << "value of x = " << x << '\n'; // x has been modified
    return 0;
}