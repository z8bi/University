#include <iostream>

int main() // assume a 32-bit application
{
    char* char_ptr{};        // chars are 1 byte
    int* int_ptr{};          // ints are usually 4 bytes
    long double* ld_ptr{};   // long doubles are usually 8 or 12 bytes

    std::cout << sizeof(char_ptr) << '\n';  // prints 8 on my computer
    std::cout << sizeof(int_ptr) << '\n';   // prints 8
    std::cout << sizeof(ld_ptr) << '\n';    // prints 8

    return 0;
}