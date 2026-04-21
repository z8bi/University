#include <iostream>
int main()
{
    int ivalue{ 5 };
    std::cout << ivalue << '\n';  // print the value of variable ivalue
    std::cout << &ivalue << '\n'; // print the memory address of variable ivalue

    double dvalue { 42 };
    std::cout << &dvalue << '\n';
    return 0;
}