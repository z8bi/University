#include <iostream>

class Base
{
protected:
    int id {};

public:
    Base() = default; // we need default constructor here, see Derived class

    Base(int id): id { id }{}

    void identify() const { std::cout << "I am a Base. ID = " << id << '\n'; }
};

class Derived : public Base
{
    double value;
public:
    Derived(double value) : value {value} {}    // no id argument
    Derived(double value, int id) : Base{id}, value {value} {}
    double get_value() const { return value; }
    void print() const {
        std::cout << "ID: " << id << ", value: " << value;
    }
};

int main()
{
    Derived d1 { 42.0 };
    Derived d2 { 24.0, 42 };
    d1.identify();
    d2.identify();
    d1.print(); std::cout << '\n';
    d2.print(); std::cout << '\n';
    return 0;
}