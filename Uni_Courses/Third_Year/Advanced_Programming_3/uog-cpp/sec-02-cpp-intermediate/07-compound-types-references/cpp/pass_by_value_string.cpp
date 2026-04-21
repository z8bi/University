#include <iostream>
#include <string>

void print_value(std::string y)
{
    std::cout << y << '\n';
} // y is destroyed here

int main()
{
    std::string x { "My name is Khiem, what's your name?" }; // x is a std::string
    print_value(x); // x is passed by value (copied) 
                    // into parameter y (expensive)
    return 0;
}