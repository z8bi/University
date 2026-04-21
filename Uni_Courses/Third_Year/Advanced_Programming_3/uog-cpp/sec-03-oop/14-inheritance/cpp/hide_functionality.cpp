#include <iostream>

class Base
{
public:
    int id {};
public:
    double public_mem {};   // this is a public member

    Base(int id) : id {id} {}
protected:
    void print_id() const { std::cout << "ID: " << id; }
};

class Derived: public Base
{
private:
    double value {};
    using Base::public_mem; // this is now private in Derived.

public: // note this public, everything below this is public now
    Derived(double value, int id) : Base {id}, value {value} {}
    double get_value() const { return value; }
    using Base::print_id;   // this is now public to the outside world
};

int main()
{   Base b { 42 };
    std::cout << "b.public_mem: " << b.public_mem << '\n'; // This is OK 
    Derived d {24.42, 42};
    // std::cout << d.public_mem;      // ERROR: public_mem is private in Derived
}