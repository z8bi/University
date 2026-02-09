#include <iostream>

template <typename T>
auto add(T x, T y) {    // Add two values with matching types
    return x + y;
}
// As of C++20 we could also use auto add(auto x, auto y)
template <typename T, typename U>   
auto add(T x, U y) {    // Add two values with non-matching types
    return x + y;
}
// As of C++20 we could also use auto add(auto x, auto y, auto z)
template <typename T, typename U, typename V>
auto add(T x, U y, V z) {   // Add three values with any type
    return x + y + z;
}

int main()
{
    std::cout << add(1.2, 3.4) << '\n'; // instantiates and calls add<double>()
    std::cout << add(5.6, 7) << '\n';   // instantiates and calls add<double, int>()
    std::cout << add(8, 9, 10) << '\n'; // instantiates and calls add<int, int, int>()
    return 0;
}