#ifndef STUDENT_H
#define STUDENT_H

#include <string>
#include <string_view>
#include "uniperson.h"

class Student : public UniPerson  // non inheritance from UniPerson
{
private:
    double tuition;
    // We can remove a lot of code here!!!
public:
    Student(std::string_view name = "", int age = 0, double tuition = 42000);
    double get_tuition() const;
};

#endif