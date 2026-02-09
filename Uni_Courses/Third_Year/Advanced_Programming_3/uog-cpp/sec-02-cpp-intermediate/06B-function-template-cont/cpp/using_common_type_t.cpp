#include <iostream>
#include <type_traits>
template <typename T, typename U>
std::common_type_t<T, U> max(T a, U b) {
    return (a < b) ? b : a;
}

int main()
{
    auto max_value = max(5, 10.5); // result will be of type double
    std::cout << "max(5, 10.5) = " << max_value << '\n';
    return 0;
}

