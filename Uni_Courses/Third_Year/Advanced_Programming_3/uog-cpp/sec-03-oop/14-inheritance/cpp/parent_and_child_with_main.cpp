#include <iostream>
class Base
{
public:
    int id {};

    Base(int id=0)
        : id { id }
    { std::cout << "Base object is constructed!\n"; }

    int get_id() const { return id; }
};

class Derived: public Base
{
public:
    double value {};

    Derived(double value=0.0)
        : value { value }
    { std::cout << "Derived object is constructed!\n"; }

    double get_value() const { return value; }
};

int main ()
{
    std::cout << "Instantiating a Parent object!\n";
    Base parent;
    std::cout << "===============================\n";
    std::cout << "Instantiating a Derived object!\n";
    Derived child;

    return 0;
}