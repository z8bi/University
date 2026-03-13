#include <iostream>

class Pennies
{
private:
	int pennies {};

public:
	Pennies(int pennies) : pennies{ pennies } { }

	// Overload Pennies + Pennies
    Pennies operator+(const Pennies& other) const;
	int get_cents() const { return pennies; }
};

// We can implement member function outside the class
// by using scope resolution operator ::
Pennies Pennies::operator+(const Pennies& other) const
{
    return Pennies {pennies + other.pennies};
}

int main()
{
	Pennies pennies_01{ 42 };
	Pennies pennies_02{ 12 };
	Pennies pennies_sum{ pennies_01 + pennies_02 };   // pennies_01.operator+(pennies_02)
	std::cout << "I have " << pennies_sum.get_cents() << " pennies.\n";
	return 0;
}