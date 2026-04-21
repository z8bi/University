#include <iostream>
#include <vector>
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
    double value {};
public:
    Derived() = default;
    Derived(double value) : value {value} {}    // no id argument
    Derived(double value, int id) : Base{id}, value {value} {}
    double get_value() const { return value; }
    void print() const {
        std::cout << "ID: " << id << ", value: " << value;
    }
};

int main()
{
    std::vector<Derived> list;
    list.push_back(Derived {42.0, 24});
    list.push_back(Derived {24.0, 42});
    list.push_back(Derived {});

    for (auto element : list) {
        element.print(); 
        std::cout << '\n';
    }
    return 0;
}