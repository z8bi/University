#include <iostream>
class Calculator
{
private:
    double value{};

public:
    Calculator(double value) : value {value} {}
    Calculator& add(double value) { this->value += value; return *this; }
    Calculator& sub(double value) { this->value -= value; return *this; }
    Calculator& mult(double value) { this->value *= value; return *this; }

    int get_value() const { return value; }
};

int main()
{
    Calculator calc {42};
    calc.add(5).sub(3).mult(4); // method chaining

    std::cout << calc.get_value() << '\n';
    std::cout << ((42 + 5) - 3) * 4 << '\n';
    return 0;
}