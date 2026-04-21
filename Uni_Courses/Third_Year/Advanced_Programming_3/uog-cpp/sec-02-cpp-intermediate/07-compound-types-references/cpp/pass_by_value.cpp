#include <iostream>

void print_value(double v)
{
    std::cout << v << '\n';
}   // v is destroyed here

int main()
{
    double x { 42.0 };

    print_value(x); // x is passed by value (copied) 
                    // into parameter v (inexpensive)
    return 0;
}