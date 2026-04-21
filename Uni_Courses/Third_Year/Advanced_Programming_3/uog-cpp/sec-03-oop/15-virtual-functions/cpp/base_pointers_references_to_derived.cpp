#include <string_view>
#include <iostream>

class Base
{
protected:
    int id {};      

public:
    Base(int id)
        : id{ id }
    {
    }

    std::string_view get_name() const { return "Base"; }
    int get_ID() const { return id; }
};

class Derived: public Base
{
private:
    double value {};
public:
    Derived(double value, int id)
        : Base{ id }, value{ value }
    {
    }

    std::string_view get_name() const { return "Derived"; }
    double get_value() const { return value; }
};

int main()
{
    Derived derived { 42.24, 42 };
    Base& base_ref { derived };
    Base* base_ptr { &derived };
    std::cout << "Name: " << derived.get_name() 
              << ", value: " << derived.get_value() << '\n';
    std::cout << "Name: " << base_ref.get_name() << '\n';
              // << ", value: " << base_ref.get_value() << '\n';
    std::cout << "Name: " << base_ptr->get_name() << '\n';
              // << ", value: " << base_ptr->get_value() << '\n';
    
    return 0;
}