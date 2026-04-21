#include <iostream>

template <typename T, typename U> // We're using two template type parameters named T and U
auto max(T x, U y) // x can resolve to type T, and y can resolve to type U
{
    return (x < y) ? y : x; // uh oh, we have a narrowing conversion problem here
}

int main()
{
    std::cout << max(3.5, 2) << '\n'; // resolves to max<int, double>

    return 0;
}