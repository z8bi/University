#include <iostream>

class A
{
public:
    A() { std::cout << "\tConstructor of A\n"; }
};

class B: public A
{
public:
    B() { std::cout << "\tConstructor of B\n"; }
};

class C: public B
{
public:
    C() { std::cout << "\tConstructor of C\n"; }
};

class D: public C
{
public:
    D() { std::cout << "\tConstructor of D\n"; }
};

int main()
{
    std::cout << "Constructing A: \n";
    A a;
    std::cout << "Constructing B: \n";
    B b;
    std::cout << "Constructing C: \n";
    C c;
    std::cout << "Constructing D: \n";
    D d;
}