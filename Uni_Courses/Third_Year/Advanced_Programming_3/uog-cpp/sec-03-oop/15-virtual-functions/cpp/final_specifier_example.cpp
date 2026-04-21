#include <string_view>

class A
{
public:
	virtual std::string_view get_name() const { return "A"; }
};

class B : public A
{
public:
	// note use of final specifier on following line -- that makes this function not able to be overridden in derived classes
	std::string_view get_name() const override final { return "B"; } // okay, overrides A::get_name()
};

class C : public B
{
public:
    // compile error: overrides B::get_name(), which is final
	// std::string_view get_name() const override { return "C"; }
};