#include <iostream>
#include <string_view>

class A
{
public:
	virtual std::string_view get_name_1(int x) { return "A"; }
	virtual std::string_view get_name_2(int x) { return "A"; }
};

class B : public A
{
public:
	virtual std::string_view get_name_1(short x) { return "B"; } // note: parameter is a short
	virtual std::string_view get_name_2(int x) const { return "B"; } // note: function is const
};

int main()
{
	B b{};
	A& base_ref{ b };
	std::cout << base_ref.get_name_1(1) << '\n';
	std::cout << base_ref.get_name_2(2) << '\n';

	return 0;
}