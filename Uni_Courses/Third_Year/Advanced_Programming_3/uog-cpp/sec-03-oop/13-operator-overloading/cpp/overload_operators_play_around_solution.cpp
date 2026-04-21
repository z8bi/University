#include <iostream>

class Pennies
{
private:
	int pennies {};

public:
	explicit Pennies(int cents) : pennies{ cents } { }

	friend Pennies operator+(const Pennies& c1, const Pennies& c2);
	friend Pennies operator-(const Pennies& c1, const Pennies& c2);
    friend int operator+(const int x, const Pennies& c);
    friend int operator+(const Pennies&, const int x);
    friend int operator*(const int x, const Pennies& c);
    friend int operator*(const Pennies& c, const int x);
	int get_cents() const { return pennies; }
};
// this function is not a member function!
Pennies operator+(const Pennies& c1, const Pennies& c2) { 
	return Pennies { c1.pennies + c2.pennies };
}

// this function is not a member function!
Pennies operator-(const Pennies& c1, const Pennies& c2) {
	return Pennies { c1.pennies - c2.pennies };
}

int operator+(const int x, const Pennies& c) {
    return (x + c.pennies);
}

int operator+(const Pennies& c, const int x) {
    return operator+(x, c);
}

int operator*(const int x, const Pennies& c) {
    return (x * c.pennies);
}
int operator*(const Pennies& c, const int x) {
    return (c.pennies * x);
}

int main()
{
	Pennies my_pennies{ 42 };
    std::cout << 42 + my_pennies << '\n';
    std::cout << my_pennies + 42 << '\n';
    std::cout << 42 * my_pennies << '\n';
    std::cout << my_pennies * 42 << '\n';
	return 0;
}