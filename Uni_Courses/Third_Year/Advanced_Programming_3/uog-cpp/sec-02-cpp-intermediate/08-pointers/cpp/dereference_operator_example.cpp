#include <iostream>

int main()
{
    double x{ 5 };
    std::cout << x << '\n';  // print the value of variable x
    std::cout << &x << '\n'; // print the memory address of variable x

    // print the value at the memory address of variable x 
    // (parentheses not required, but make it easier to read)
    std::cout << *(&x) << '\n'; 

    return 0;
}