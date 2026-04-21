#include <iostream>

int add(int x, int y) {// integer version
    return x + y;
}

double add(double x, double y) {// floating point version
    return x + y;
}

int main()
{
    int x { add(1, 2) };
    double y { add(42.24, 24.42) };
    std::cout << "x = " << x << '\n';
    std::cout << "y = " << y << '\n';
    return 0;
}