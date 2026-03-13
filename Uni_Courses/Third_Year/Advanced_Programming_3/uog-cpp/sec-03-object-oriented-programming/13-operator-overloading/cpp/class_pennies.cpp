class Pennies
{
private:
	int pennies {};

public:
	Pennies(int pennies) : pennies{ pennies } { }
	int get_pennies() const { return pennies; }
};