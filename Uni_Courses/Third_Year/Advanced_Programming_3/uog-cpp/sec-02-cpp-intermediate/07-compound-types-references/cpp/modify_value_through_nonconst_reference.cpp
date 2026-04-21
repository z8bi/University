#include <iostream>

int main()
{
    int x { 42 };            // normal integer variable
    int& ref { x };         // ref is now an alias for variable x

    std::cout << x << ref << '\n';  

    x = 24;                             // x now has value 24

    std::cout << x << ref << '\n';      

    ref = 12;    // the object being referenced (x) now has value 12

    std::cout << x << ref << '\n';

    return 0;
}