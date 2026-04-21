#include <iostream>
#include <string>
void print_value(std::string& y) // type changed to std::string&
{
    std::cout << y << '\n';
} // y is destroyed here
int main()
{
    std::string x { "Hello, world!" };
    print_value(x); // x is now passed by reference into 
                    // reference parameter y (inexpensive)
    return 0;
}