#include <iostream>

class ComplexNumber
{
    double real;
    double imag;
public:
    ComplexNumber(double real, double imag) : real {real}, imag {imag} {}

    ComplexNumber operator+(const ComplexNumber& other) const {
        return ComplexNumber(real + other.real, imag + other.imag);
    }

    friend std::ostream& operator<<(std::ostream& out, const ComplexNumber& a) {
        out << a.real << " + " << a.imag << "*i";
        return out;
    }
};

int main()
{
    ComplexNumber a {42, 24};
    ComplexNumber b {24, 42};
    std::cout << a << '\n';     // these work now
    std::cout << b << '\n';
    std::cout << a + b << '\n';
    return 0;
}