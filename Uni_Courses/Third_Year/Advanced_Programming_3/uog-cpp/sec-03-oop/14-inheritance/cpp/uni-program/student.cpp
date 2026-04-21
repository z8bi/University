#include "student.h"

Student::Student(std::string_view name, int age, double tuition) 
    : UniPerson{name, age}, tuition{tuition} 
    {}

double Student::get_tuition() const { 
    return tuition; 
}