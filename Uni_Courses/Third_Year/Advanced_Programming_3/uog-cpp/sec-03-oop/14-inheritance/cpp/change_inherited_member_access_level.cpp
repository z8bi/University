#include <iostream>

class Base
{
private:
    int id {};
public:
    Base(int id) : id {id} {}
protected:
    void print_id() const { std::cout << "ID: " << id; }
};

class Derived: public Base
{
private:
    double value {};

public: // note this public, everything below this is public now
    Derived(double value, int id) : Base {id}, value {value} {}
    double get_value() const { return value; }
    //Base::print_id() was inherited as protected, so the public has no access.
    // But we are changing it to public via a using declaration
    using Base::print_id;   // note: no parenthesis here
};

int main()
{
    Derived d {24.42, 42};
    d.print_id();      // prints 7
    std::cout << ", d.value: " << d.get_value() << '\n';
}