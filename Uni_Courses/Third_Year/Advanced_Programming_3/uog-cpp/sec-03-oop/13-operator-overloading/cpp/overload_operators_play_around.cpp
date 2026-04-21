#include <iostream>

class Pennies
{
private:
	int pennies {};

public:
	explicit Pennies(int cents) : pennies{ cents } { }

	friend Pennies operator+(const Pennies& c1, const Pennies& c2);
	friend Pennies operator-(const Pennies& c1, const Pennies& c2);
	friend Pennies operator+(int value, const Pennies& another);
	friend Pennies operator+(const Pennies& another, int value);
	int get_cents() const { return pennies; }
};
// this function is not a member function!
Pennies operator+(const Pennies& c1, const Pennies& c2) { 

	/// 200 lines of codes
	return Pennies { c1.pennies + c2.pennies };
}

Pennies operator+(int value, const Pennies& another) {
	/// 200 lines of codes
	// return Pennies { value + another.pennies };
	return operator+(another, value);
}

Pennies operator+(const Pennies& another, int value) {
	return Pennies { value + another.pennies };
}

// this function is not a member function!
Pennies operator-(const Pennies& c1, const Pennies& c2) {
	return Pennies { c1.pennies - c2.pennies };
}

int main()
{
	Pennies my_cents{ 42 };
    
    std::cout << 42 + my_cents;
    /* 
	std::cout << my_cents + 42;
    std::cout << 42 * my_cents;
    */
	return 0;
}