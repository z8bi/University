#include <iostream>
class Base
{
private:
	int id {};

public:
	Base(int id) : id { id } { }
	int get_id() const { return id; }
    void print_base() const { 
        std::cout << "Base object with id: " << id; 
    }
};

class Derived : public Base
{
    double value {};
public:
	Derived(double value, int id) : Base {id}, value {value} {}
    double get_value() const { return value; };

    // it does not make sense to print a Derived-type object 
    // using the print_base() for a Base-type object
	void print_base() const = delete; // mark this function as inaccessible
};

int main()
{
	Derived derived { 42.24, 42 };
	std::cout << derived.get_id() << '\n';  // this is fine.

    // ERROR: This function has been deleted in Derived
    // std::cout << derived.print_base();  

	return 0;
}