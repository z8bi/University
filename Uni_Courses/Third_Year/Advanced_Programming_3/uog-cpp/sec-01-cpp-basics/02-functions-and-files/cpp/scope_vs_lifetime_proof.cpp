#include <iostream>

void second_function(int* pointer_to_x) {
    // We cannot use the name 'x' here (Out of Scope)
    // But we can look at the memory address we were given
    std::cout << "Inside second_function:" << std::endl;
    std::cout << "Value at memory address: " << *pointer_to_x << std::endl;
    std::cout << "Memory address: " << pointer_to_x << std::endl;
    // std::cout << x;
}

void first_function() {
    int x = 10; // 'x' starts its lifetime here
    // We pass the ADDRESS of x (&x) to the next function
    second_function(&x); 
} // 'x' dies here

int main() {
    first_function();
    return 0;
}