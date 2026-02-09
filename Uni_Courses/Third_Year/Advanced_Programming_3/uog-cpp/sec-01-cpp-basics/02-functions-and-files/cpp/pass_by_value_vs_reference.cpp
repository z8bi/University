#include <iostream>

void modify_value(int x) {
    x = 20; // This modification will not affect the original variable
    std::cout << "Inside modify_value, x = " << x << std::endl;
}

void modify_value_ref(int &x) {
    x = 30; // This modification will affect the original variable
}

int main() 
{
    int value { 10 };

    std::cout << "Before modify_value, value = " << value << std::endl;
    modify_value(value);        // Pass by value
    std::cout << "After modify_value, value = " << value << std::endl;

    modify_value_ref(value);
    std::cout << "After modify_value_ref, value = " << value << std::endl;clear
    return 0;
}