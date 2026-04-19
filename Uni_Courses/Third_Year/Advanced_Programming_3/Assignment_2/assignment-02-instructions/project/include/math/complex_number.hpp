#ifndef COMPLEX_NUMBER_HPP
#define COMPLEX_NUMBER_HPP

#include <iostream>
#include <vector>
#include <initializer_list>
#include <cmath>

namespace math
{

    class ComplexNumber {
    protected:
        double re;
        double im;

    public:
        ComplexNumber(double r = 0.0, double i = 0.0) : re(r), im(i) {}

        ComplexNumber& operator=(const ComplexNumber& other) {
            if (this != &other) {
                re = other.re;
                im = other.im;
            }
            return *this;
        }

        ComplexNumber operator+(const ComplexNumber& other) const {
            return ComplexNumber(re + other.re, im + other.im);
        }

        ComplexNumber operator*(const ComplexNumber& other) const {
            return ComplexNumber(
                re * other.re - im * other.im,
                re * other.im + im * other.re
            );
        }

        bool operator==(const ComplexNumber& other) const {
            return (re == other.re && im == other.im);
        }

        bool operator!=(const ComplexNumber& other) const {
            return !(*this == other);
        }

        ComplexNumber pow(int n) const {
            ComplexNumber result(1.0, 0.0);
            for (int i = 0; i < n; ++i) {
                result = result * (*this);
            }
            return result;
        }

        friend std::ostream& operator<<(std::ostream& os, const ComplexNumber& cn) {
            if (cn.re != 0 && cn.im > 0)
                os << cn.re << " + " << cn.im << "i";
            else if (cn.re != 0 && cn.im < 0)
                os << cn.re << " - " << -cn.im << "i";
            else if (cn.re == 0 && cn.im != 0)
                os << cn.im << "i";
            else
                os << cn.re;
            return os;
        }

        double real() const { return re; }
        double imag() const { return im; }
    };

    // --- Factory Functions ---

    /**
     * Creates a vector of ComplexNumbers with value 1 + 0i
     */
    inline std::vector<ComplexNumber> ones(size_t shape) {
        return std::vector<ComplexNumber>(shape, ComplexNumber(1.0, 0.0));
    }

    /**
     * Creates a vector of ComplexNumbers with value 0 + 0i
     */
    inline std::vector<ComplexNumber> zeros(size_t shape) {
        return std::vector<ComplexNumber>(shape, ComplexNumber(0.0, 0.0));
    }

    /**
     * Creates a vector of ComplexNumbers with value 0 + 1i
     */
    inline std::vector<ComplexNumber> imaginary_units(size_t shape) {
        return std::vector<ComplexNumber>(shape, ComplexNumber(0.0, 1.0));
    }

    /**
     * Creates a vector of ComplexNumbers with values linearly spaced between start and end
     */
    inline std::vector<ComplexNumber> linspace(
        const ComplexNumber& start, 
        const ComplexNumber& end, 
        size_t num) 
    {
        std::vector<ComplexNumber> result;
        if (num == 0) return result;
        if (num == 1) {
            result.push_back(start);
            return result;
        }
        ComplexNumber step((end.real() - start.real()) / (num - 1), (end.imag() - start.imag()) / (num - 1));
        for (size_t i = 0; i < num; ++i) {
            result.push_back(ComplexNumber(start.real() + i * step.real(), start.imag() + i * step.imag()));
        }
        return result;
    }

    /**
     * Compute the magnitude of a complex number
     */
    inline double compute_magnitude(const ComplexNumber& number) {
        return std::sqrt(std::pow(number.real(), 2) + std::pow(number.imag(), 2));
    }
} // namespace numcpp

#endif // COMPLEX_NUMBER_HPP