/*
If you use this to compile, then you don't need
g++ -I /path/to/include/ main_complex.cpp

#include "../include/math/poly1d.hpp"          // Path relative to this file
#include "../include/math/complex_number.hpp"  // Path relative to this file
*/


#include "math/poly1d.hpp"
#include "math/complex_number.hpp"  // Then you need to specify g++ -I /path/to/include
#include <iostream>

int main() {
    math::Poly1D<double> p({1, -6, 11, -6}); // Represents x^3 - 6x^2 + 11x - 6
    std::cout << "Polynomial p: " << p << std::endl;

    double x = 2.0;
    std::cout << "p(" << x << ") = " << p(x) << std::endl;

    math::Poly1D<double> dp = p.derive();
    std::cout << "Derivative: " << dp << std::endl;
    std::cout << "p'(" << x << ") = " << dp(x) << std::endl;

    math::Poly1D<double> q({-1, 0, 1}); // Represents -x^2 + 1
    std::cout << "Another Polynomial q: " << q << std::endl;

    math::Poly1D<double> r = p + q;
    std::cout << "Sum of Polynomials p + q: " << r << std::endl;
    std::cout << "r(" << x << ") = " << r(x) << std::endl;

    double root_guess = 1.5;
    double root1 = p.find_root(root_guess);
    std::cout << "Root of p(x) = 0 found near " << root_guess << ": " << root1 
              << ", p(root1) = " << p(root1) << std::endl;


    double root2 = math::find_root(p, root_guess); // Using the standalone function as well
    std::cout << "Root of p(x) = 0 found near " << root_guess << ": " << root2 
              << ", p(root2) = " << p(root2) << std::endl;


    math::Poly1D<math::ComplexNumber> cp(
            {math::ComplexNumber(1, 1), 
            math::ComplexNumber(-2, 0), 
            math::ComplexNumber(1, -1)});
    std::cout << "Complex Polynomial cp: " << cp << std::endl;
    return 0;
}

/* Compile this file by using the command line:

(1) If your working directory is project/examples
    g++ -I ../include/ main_poly1d.cpp -o poly.out
(2) If your working directory is project
    g++ -I ./include main_poly1.cpp -o poly.out
*/