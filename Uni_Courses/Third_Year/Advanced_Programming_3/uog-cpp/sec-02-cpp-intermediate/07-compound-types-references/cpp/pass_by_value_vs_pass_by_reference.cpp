#include <iostream>

void print_addresses(double val, double& ref)
{
    std::cout << "The address of the value parameter is    : " << &val << '\n';
    std::cout << "The address of the reference parameter is: " << &ref << '\n';
}
int main()
{
    double x { 42.0 };
    std::cout << "The address of x is: " << &x << '\n';
    print_addresses(x, x);
    return 0;
}