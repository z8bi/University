#include <iostream>

int add(int x, int y) { return x + y; }
double add(double x, double y) { return x + y; }
double add(double x, double y, double z) { return x + y + z; }

int main()
{
    std::cout << add(1, 2) << '\n'; // calls add(int, int)

    std::cout << add(1.2, 3.4) << '\n'; // calls add(double, double)
    
    std::cout << add(1, 2.1f, 1.2) << '\n';  // calls add(double, double, double)
    // 1 is promoted to double 1.0, 2.1f is promoted to double 2.1
    return 0;
}