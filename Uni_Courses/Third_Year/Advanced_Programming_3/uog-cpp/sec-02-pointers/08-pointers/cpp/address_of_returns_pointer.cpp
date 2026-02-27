#include <iostream>
#include <typeinfo>

int main()
{
	double x{ 42.0 };
	std::cout << typeid(x).name() << '\n';  // print the type of x
	std::cout << typeid(&x).name() << '\n'; // print the type of &x

	return 0;
}