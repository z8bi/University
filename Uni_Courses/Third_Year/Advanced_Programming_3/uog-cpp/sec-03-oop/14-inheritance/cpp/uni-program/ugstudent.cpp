#include <iostream>
#include "ugstudent.h"

// Remember: No default arguments here!
UGStudent::UGStudent(std::string_view name, int age, double tuition)
    : Student{name, age, tuition} {}

void UGStudent::write_BSc_thesis() const {
    std::cout << "Student (ID: " << get_id() << ") is writing the thesis" << std::endl;
}