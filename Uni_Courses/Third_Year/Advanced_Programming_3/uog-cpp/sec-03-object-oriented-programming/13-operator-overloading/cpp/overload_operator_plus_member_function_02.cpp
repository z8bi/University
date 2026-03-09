#include <iostream>

class Pennies
{
private:
	int pennies {};

public:
	Pennies(int cents) : pennies{ cents } { }

	// Overload Pennies + Pennies
    Pennies operator+(const Pennies& other) const;
    Pennies operator+(int value) const;
	int get_cents() const { return pennies; }

    friend std::ostream& operator<<(std::ostream& out, const Pennies& p);
};

// We can implement member function outside the class
// by using scope resolution operator ::
Pennies Pennies::operator+(const Pennies& other) const
{
    return Pennies {pennies + other.pennies};
}

Pennies Pennies::operator+(int value) const
{
    return Pennies {pennies + value};
}

std::ostream& operator<<(std::ostream& out, const Pennies& p)
{
    out << p.pennies << " pennies";
    return out;
}

int main()
{
	Pennies cents1{ 42 };
	Pennies cents2{ 12 };
	Pennies cents_sum{ cents1 + cents2 };   // cents1.operator+(cents2)
	std::cout << "I have " << cents_sum.get_cents() << " pennies.\n";
    std::cout << "I have " << (cents1 + 12) << '.\n';
	return 0;
}