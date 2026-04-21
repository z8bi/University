#include <iostream>
class A
{
public:
    A(int a) { std::cout << "A: " << a << '\n'; }
    ~A() { std::cout << "A-type object is destroyed!\n"; }
};

class B: public A
{
public:
    B(int a, double b) : A{ a } {
        std::cout << "B: " << b << '\n';
    }
    ~B() { std::cout << "B-type object is destroyed!\n"; }
};

class C: public B
{
public:
    C(int a, double b, char c) : B{ a, b } {
        std::cout << "C: " << c << '\n';
    }
    ~C() { std::cout << "C-type object is destroyed!\n"; }
};

int main()
{
    C c{ 5, 4.3, 'R' };
    return 0;
}