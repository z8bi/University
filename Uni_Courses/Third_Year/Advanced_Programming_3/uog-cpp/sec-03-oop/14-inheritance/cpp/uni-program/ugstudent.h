#ifndef UGSTUDENT_H
#define UGSTUDENT_H

#include "student.h"

class UGStudent : public Student {
public:
    // Call the base Student constructor
    UGStudent(std::string_view name = "", int age = 0, double tuition = 0.0);
    
    void write_BSc_thesis() const;
};

#endif