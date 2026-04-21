#include <iostream>

void print_ref(const int& y) // y is a const reference
{
    std::cout << y << '\n';
}

int main()
{
    int x { 5 };
    print_ref(x);   // ok: x is a modifiable lvalue, y binds to x

    const int z { 5 };
    print_ref(z);   // ok: z is a non-modifiable lvalue, y binds to z
    print_ref(5);   // ok: 5 is rvalue literal, y binds to temporary int object
    return 0;
}