#include <iostream>

class ComplexNumber
{
    double real;
    double imag;
public:
    ComplexNumber(double real, double imag) : real {real}, imag {imag} {}
};

int main()
{
    ComplexNumber a {42, 24};
    ComplexNumber b {24, 42};
    /*
    std::cout << a << '\n';     // these won't work.
    std::cout << b << '\n';
    std::cout << a + b << '\n';
    */
}