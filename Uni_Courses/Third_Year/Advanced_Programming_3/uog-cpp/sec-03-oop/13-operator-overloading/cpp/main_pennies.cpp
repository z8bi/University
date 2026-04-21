#include "pennies/pennies.h"
#include <iostream>

int main()
{
    Pennies p1 { 42 };
    Pennies p2 { 24 };

    // without the prototype in Pennies.h, this would fail to compile
    Pennies pennies_sum { p1 + p2 };

    std::cout << "I have " << pennies_sum.get_pennies() << " pennies.\n";

    return 0;
}