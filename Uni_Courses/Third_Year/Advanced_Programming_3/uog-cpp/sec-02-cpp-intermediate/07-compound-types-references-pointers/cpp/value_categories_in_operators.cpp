#include <iostream>

int main()
{
    std::cout << 1 + 2; // 1 and 2 are rvalues, operator+ returns an rvalue

    // But what about this -- did we learn that x and y should be lvalues?:
    double x { 24.1 };
    double y { 42.2 };
    std::cout << x + y;     // x and y are rvalues in the expression x + y.
    return 0;
}