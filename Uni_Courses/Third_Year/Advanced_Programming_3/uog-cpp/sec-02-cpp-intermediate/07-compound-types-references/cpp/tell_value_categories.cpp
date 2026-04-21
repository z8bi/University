#include <iostream>
#include <string>

// T& is an lvalue reference, so this overload will be preferred for lvalues
template <typename T>
constexpr bool is_lvalue(T&)
{
    return true;
}

// T&& is an rvalue reference, so this overload will be preferred for rvalues
template <typename T>
constexpr bool is_lvalue(T&&)
{
    return false;
}

// A helper macro (#expr prints whatever is passed in for expr as text)
#define PRINT_VALUE_CATEGORY(expr) { std::cout << #expr << " is an " << (is_lvalue(expr) ? "lvalue\n" : "rvalue\n"); }

int get_int() { return 5; }

int main()
{
    PRINT_VALUE_CATEGORY(5);           // rvalue
    PRINT_VALUE_CATEGORY(get_int());   // rvalue
    int x { 5 };
    PRINT_VALUE_CATEGORY(x);                        // lvalue
    PRINT_VALUE_CATEGORY(std::string {"Hello"});    // rvalue
    PRINT_VALUE_CATEGORY("Hello");                  // lvalue
    PRINT_VALUE_CATEGORY(++x);                      // lvalue
    PRINT_VALUE_CATEGORY(x++);                      // rvalue
}