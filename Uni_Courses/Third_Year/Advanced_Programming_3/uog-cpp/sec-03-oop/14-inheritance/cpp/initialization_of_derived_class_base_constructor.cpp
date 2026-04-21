#include <iostream>
class Base
{
public:
    const int id {};  

    Base(int id=0) : id{ id }  {}

    int get_id() const { return id; }
    void print() const {
        std::cout << "Base:\n\tId: " << id;
    }
};

class Derived: public Base
{
public:
    double value {};

    Derived(double value=0.0, int id=0) 
        : Base{id}, value{ value } {}

    double get_value() const { return value; }
    void print() const {    // this will override void Base::print()
        std::cout << "Derived:\n\tId: " << id << '\n';
        std::cout << "\tValue: " << value;
    }
};

int main()
{   
    Base base{ 4224 };
    base.print();
    Derived derived{ 42, 24 };
    derived.print();
    std::cout << '\n';
    return 0;
}