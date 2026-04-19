#pragma once

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <type_traits>

namespace math {
    template <typename T>
    class Poly1D {
    private:
        // Coefficients stored from highest degree to lowest (NumPy style)
        // [1, 2, 3] -> 1x^2 + 2x + 3
        std::vector<T> coeffs;

    public:
        Poly1D(std::vector<T> c) : coeffs(c) {
            // Remove leading zeros to maintain correct degree
            // Uses T(0) to ensure compatibility with ComplexNumber
            auto it = coeffs.begin();
            while (it != coeffs.end() - 1 && *it == T(0)) {
                it = coeffs.erase(it);
            }
        }

        // Degree of the polynomial
        int degree() const {
            return coeffs.empty() ? 0 : static_cast<int>(coeffs.size()) - 1;
        }

        // Evaluation using Horner's Method: p(x) = (...((a_n*x + a_{n-1})*x + a_{n-2})...)
        // Input and Output changed to T to support complex evaluation
        T operator()(T x) const {
            T result(0);
            for (const T& c : coeffs) {
                result = result * x + c;
            }
            return result;
        }

        // Derivative: if P(x) = ax^2 + bx + c, P'(x) = 2ax + b
        Poly1D derive() const {
            if (coeffs.size() <= 1) return Poly1D({T(0)});
            std::vector<T> d_coeffs;
            int n = degree();
            for (int i = 0; i < n; ++i) {
                // static_cast to double allows multiplication if T supports it (like ComplexNumber)
                d_coeffs.push_back(coeffs[i] * static_cast<double>(n - i));
            }
            return Poly1D(d_coeffs);
        }

        // Addition
        Poly1D operator+(const Poly1D& other) const {
            int max_size = std::max(coeffs.size(), other.coeffs.size());
            std::vector<T> res(max_size, T(0));
            
            for (int i = 0; i < max_size; ++i) {
                int idx1 = static_cast<int>(coeffs.size()) - 1 - i;
                int idx2 = static_cast<int>(other.coeffs.size()) - 1 - i;
                if (idx1 >= 0) res[max_size - 1 - i] = res[max_size - 1 - i] + coeffs[idx1];
                if (idx2 >= 0) res[max_size - 1 - i] = res[max_size - 1 - i] + other.coeffs[idx2];
            }
            return Poly1D(res);
        }

        // Subtraction
        Poly1D operator-(const Poly1D& other) const {
            int max_size = std::max(coeffs.size(), other.coeffs.size());
            std::vector<T> res(max_size, T(0));
            
            for (int i = 0; i < max_size; ++i) {
                int idx1 = static_cast<int>(coeffs.size()) - 1 - i;
                int idx2 = static_cast<int>(other.coeffs.size()) - 1 - i;
                // Note: Assumes T supports binary minus or addition of negative
                if (idx1 >= 0) res[max_size - 1 - i] = res[max_size - 1 - i] + coeffs[idx1];
                if (idx2 >= 0) res[max_size - 1 - i] = res[max_size - 1 - i] - other.coeffs[idx2];
            }
            return Poly1D(res);
        }

        // Multiplication
        Poly1D operator*(const Poly1D& other) const {
            std::vector<T> res(degree() + other.degree() + 1, T(0));
            for (size_t i = 0; i < coeffs.size(); ++i) {
                for (size_t j = 0; j < other.coeffs.size(); ++j) {
                    res[i + j] = res[i + j] + (coeffs[i] * other.coeffs[j]);
                }
            }
            return Poly1D(res);
        }

        /**
         * Redesigned Output Formatting
         * Handles complex numbers by wrapping them in () and using '+' as a universal separator.
         */
        friend std::ostream& operator<<(std::ostream& os, const Poly1D<T>& p) {
            int n = p.degree();
            bool first_printed = false;

            for (size_t i = 0; i < p.coeffs.size(); ++i) {
                int pwr = n - i;
                const T& c = p.coeffs[i];

                // Skip zero terms unless it's the only term in the polynomial
                if (c == T(0) && n > 0) continue;
                
                if (first_printed) os << " + ";

                // formatting: wrap complex numbers in parentheses, print doubles normally
                if constexpr (std::is_floating_point_v<T>) {
                    os << c;
                } else {
                    os << "(" << c << ")";
                }
                
                if (pwr > 0) os << "x";
                if (pwr > 1) os << "^" << pwr;

                first_printed = true;
            }
            return os;
        }

        // Newton's Method solver
        // Note: For T = ComplexNumber, this requires operator/ to be implemented in complex_number.hpp
        // We don't try to implement a fully functional code here.
        T find_root(T x0, double tol = 1e-6, int max_iter = 500) const {
            T x = x0;
            Poly1D<T> p_prime = this->derive();
            
            for (int i = 0; i < max_iter; ++i) {
                T fx = (*this)(x);
                T fpx = p_prime(x);
                
                // Simplified zero check; for Complex, consider adding a magnitude/abs function
                if (fpx == T(0)) break; 
                
                T x_next = x - (fx / fpx);
                
                // Using a generic approach to check convergence
                // This assumes T can be subtracted and compared to a tolerance
                if constexpr (std::is_floating_point_v<T>) {
                    if (std::abs(x_next - x) < tol) return x_next;
                }
                // For complex numbers, you'd ideally check if absolute magnitude < tol
                
                x = x_next;
            }
            return x;
        }
    };

    // Helper function for Newton's Method
    template <typename T>
    T find_root(const Poly1D<T>& p, T x0, double tol = 1e-6, int max_iter = 500) {
        return p.find_root(x0, tol, max_iter);
    }

} // namespace math