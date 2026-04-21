#ifndef LECTURER_H
#define LECTURER_H

#include <string>
#include <string_view>
#include "uniperson.h"

class Lecturer : public UniPerson  // non inheritance from UniPerson
{
private:
    double salary;
    // We can remove a lot of code here!!!
public:
    Lecturer(std::string_view name = "", int age = 0, double salary = 42000): 
        UniPerson{name, age}, // we borrowed the constructor from UniPerson above
        salary {salary}
    {
    };
    double get_salary() const { return salary; }
    // We can remove a lot of code for member functions here.
};
// We don't need this code anymore and it makes perfect sense.
// int Lecturer::nextID{10000};    

#endif