#include <iostream>

class Pennies
{
private:
	int pennies {};

public:
    // Keyword "explicit" prevents the compiler from performing 
    // unintended automatic type conversions.
    // We don't want 42.24 pennies. Cent is already a smallest unit
    // in money
	explicit Pennies(int pennies) : pennies{ pennies } { }     

	// add Pennies + Pennies using a friend function
    // This function is not considered a member of the class, 
    // even though the definition is inside the class
	friend Pennies operator+(const Pennies& c1, const Pennies& c2)
	{
		// use the Pennies constructor and operator+(int, int)
		// we can access pennies directly because this is a friend function
		return Pennies { c1.pennies + c2.pennies };
	}

	int get_pennies() const { return pennies; }
};

int main()
{
	Pennies pennies_01{ 42 };
	Pennies pennies_02{ 24 };
	Pennies pennies_sum{ pennies_01 + pennies_02 };
	std::cout << "I have " << pennies_sum.get_pennies() << " pennies.\n";
    
	return 0;
}