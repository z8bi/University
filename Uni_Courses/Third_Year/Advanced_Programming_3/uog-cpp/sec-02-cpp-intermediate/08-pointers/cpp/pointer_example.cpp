#include <iostream>

int main()
{
    double x { 42 };    // normal variable
    double& ref { x }; // a reference to a double (bound to x)

    double* ptr1;       // a pointer to a double
    ptr1 = &x;
    std::cout << "ptr1 = " << ptr1 << '\n';   // show some address
    std::cout << "value stored at ptr1 = " << *ptr1 << '\n';

    double* ptr2 = &ref;
    std::cout << "ptr2 = " << ptr2 << '\n';
    std::cout << "value stored at ptr2 = " << *ptr2 << '\n'; 
    
    return 0;
}