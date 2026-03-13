#include <iostream>

class ComplexNumber {
private:
    double re{};
    double im{};

public:
    ComplexNumber(double re = 0, double im = 0) : re(re), im(im) {}

    void set_num(double new_re, double new_im) {
        this->re = new_re;
        this->im = new_im;
    }

    void print_num() const {
        std::cout << re << " + " << im << "i" << std::endl;
    }
};

int main() {

    ComplexNumber my_num(1, 2);
    my_num.print_num();
    
    return 0;
}