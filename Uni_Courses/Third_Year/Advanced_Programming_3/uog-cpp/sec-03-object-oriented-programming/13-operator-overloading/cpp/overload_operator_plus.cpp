#include <iostream>

class Pennies
{
private:
	int pennies {};

public:
	Pennies(int cents) : pennies{ cents } { }

	// add Pennies + Pennies using a friend function
	friend Pennies operator+(const Pennies& c1, const Pennies& c2);

	int get_cents() const { return pennies; }
};

// note: this function is not a member function!
Pennies operator+(const Pennies& c1, const Pennies& c2)
{
	// use the Pennies constructor and operator+(int, int)
	// we can access cents directly because this is a friend function
	return c1.pennies + c2.pennies;
}

int main()
{
	Pennies cents1{ 42 };
	Pennies cents2{ 13 };
	Pennies cents_sum{ cents1 + cents2 };
	std::cout << "I have " << cents_sum.get_cents() << " cents.\n";
	return 0;
}