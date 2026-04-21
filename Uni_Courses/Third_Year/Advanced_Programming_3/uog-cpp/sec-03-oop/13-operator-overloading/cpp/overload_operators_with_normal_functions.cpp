#include <iostream>

class Pennies
{
private:
	int pennies {};

public:
	explicit Pennies(int pennies) : pennies{ pennies } { }     

    // we can access internal data by a getter function.
	int get_pennies() const { return pennies; }
};

// note: this function is not a member function nor a friend function!
Pennies operator+(const Pennies& p1, const Pennies& p2) {
    return Pennies{p1.get_pennies() + p2.get_pennies()};
}

int main()
{
	Pennies pennies_01{ 42 };
	Pennies pennies_02{ 24 };
	Pennies pennies_sum{ pennies_01 + pennies_02 };
	std::cout << "I have " << pennies_sum.get_pennies() << " pennies.\n";
    
	return 0;
}