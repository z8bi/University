/*
If you use this to compile, then you don't need
g++ -I /path/to/include/ main_complex.cpp

#include "../include/math/complex_number.hpp" // Path relative to this file
*/

#include "math/complex_number.hpp"  // We need g++ -I /path/to/include if we write this
#include <iostream>

int main() {

    // Create a vector of ComplexNumber objects with value 0 + 1i
    std::vector<math::ComplexNumber> numbers 
        = math::linspace(math::ComplexNumber(1, 2), math::ComplexNumber(5, 6), 5);

    std::cout << "Linspace: \n";
    for (const auto& num : numbers) {
        std::cout << num << "\t|\t" << "magnitude = " << math::compute_magnitude(num) << '\n';
    }
    std::cout << std::endl;

    math::ComplexNumber s(0, 0);
    for (const auto& num : numbers) {
        s = s + num;
    }

    std::cout << "Sum of array: " << s << "\n\n";

    // Lambda calculation: 1 + z + z^2 + z^3 + z^4 + z^5
    math::ComplexNumber z(1, 2);
    std::cout << "z = " << z << std::endl;
    math::ComplexNumber lambda(0, 0);
    for (int i = 0; i <= 5; ++i) {
        lambda = lambda + z.pow(i);
    }

    std::cout << "Lambda (1+z+...+z^5): " << lambda << std::endl;

    return 0;
}

/* Compile this file by using the command line:

(1) If your working directory is project/examples
    g++ -I ../include/ main_complex.cpp -o complex.out
(2) If your working directory is project
    g++ -I ./include main_complex.cpp -o complex.out
*/
