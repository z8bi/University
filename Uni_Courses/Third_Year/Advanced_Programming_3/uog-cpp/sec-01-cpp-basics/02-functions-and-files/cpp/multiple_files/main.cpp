#include <iostream>

// needed so main.cpp knows that add() is a function defined elsewhere
float add(float x, float y);

int main()
{
    std::cout << "The sum of 3 and 4 is: " << add(3.0, 4.0) << '\n';
    return 0;
}