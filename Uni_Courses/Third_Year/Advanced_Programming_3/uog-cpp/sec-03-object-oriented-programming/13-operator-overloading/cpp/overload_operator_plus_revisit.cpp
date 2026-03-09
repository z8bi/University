#include <iostream>

class Pennies
{
private:
	int pennies {};

public:
	Pennies(int cents) : pennies{ cents } { }

	friend Pennies operator+(const Pennies& c1, const Pennies& c2);

	int get_cents() const { return pennies; }
};

Pennies operator+(const Pennies& c1, const Pennies& c2)
{
	return c1.pennies + c2.pennies;
}

int main()
{
	Pennies cents1{ 42 };
	Pennies cents2{ 12 };
	Pennies cents_sum{ cents1 + cents2 };
	std::cout << "I have " << cents_sum.get_cents() << " cents.\n";
	return 0;
}