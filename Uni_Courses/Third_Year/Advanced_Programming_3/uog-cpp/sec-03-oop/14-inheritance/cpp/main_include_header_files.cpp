#include <iostream>
#include <string>
#include <string_view>
#include "uniperson.h"
#include "lecturer.h"

// We don't need this code anymore and it makes perfect sense.
// int Lecturer::nextID{10000};    
int main()
{
    Lecturer khiem {"khiem", 42};
    std::cout << "Khiem's age is " << khiem.get_age() << '\n';
    std::cout << "Khiem's ID is " << khiem.get_id() << '\n';
    
    Lecturer scott {"Scott", 101}; // his hair tells me that
    std::cout << "Scott's age is " << scott.get_age() << '\n';
    std::cout << "Scott's ID is " << scott.get_id() << '\n';
}