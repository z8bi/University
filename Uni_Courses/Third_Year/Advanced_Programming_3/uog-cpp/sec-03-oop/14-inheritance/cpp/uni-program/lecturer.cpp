#include "lecturer.h"

Lecturer::Lecturer(std::string_view name, int age, double salary) 
    : UniPerson{name, age}, salary{salary} {}

double Lecturer::get_salary() const { 
    return salary; 
}