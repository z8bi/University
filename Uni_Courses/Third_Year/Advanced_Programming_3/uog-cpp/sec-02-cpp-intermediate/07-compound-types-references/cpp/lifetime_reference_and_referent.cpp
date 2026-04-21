#include <iostream>

int main()
{
    int x { 42 };

    {   // note these curly braces
        int& ref { x };             // ref is a reference to x
        std::cout << ref << '\n';   // prints value of ref (42)
    }   // ref is destroyed here -- x is unaware of this

    std::cout << x << '\n'; // prints value of x (5)

    return 0;
}   // x destroyed here