#include <iostream>

int main()
{
    double x{ 42 };
    double* ptr{ &x };          // initialize ptr with address of variable x

    std::cout << x << '\n';     // print x's value
    std::cout << *ptr << '\n';  // print the value at the address that ptr is 
                                // holding (x's address)

    *ptr = 24;  // The object at the address held by ptr (x) 
                // assigned value 24 (note that ptr is dereferenced here)

    std::cout << x << '\n';
    std::cout << *ptr << '\n';  // print the value at the address that ptr is 
                                // holding (x's address)

    return 0;
}