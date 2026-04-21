#include <iostream>
#include "lecturer.h"
#include "student.h"
#include "ugstudent.h"

int main() {
    Lecturer khiem{"Khiem", 45, 55000};
    Student tony_stark {"Tony Stark", 20, 90000};   // he is rich, so pay a lot
    UGStudent black_widow {"Scatlet", 20, 900};     // she is widow, so discount   
    std::cout << khiem.get_name() << " (ID: " 
              << khiem.get_id() << ") Salary: " 
              << khiem.get_salary() << "\n";
    std::cout << tony_stark.get_name() 
              << " (ID: " << tony_stark.get_id() 
              << ") Tuition: " << tony_stark.get_tuition() << "\n";
    
    std::cout << black_widow.get_name() 
              << " (ID: " << black_widow.get_id() 
              << ") Tuition: " << black_widow.get_tuition() << "\n";

    black_widow.write_BSc_thesis();
    return 0;
}

// Compile the program
// g++ -std=c++17 main.cpp uniperson.cpp lecturer.cpp student.cpp ugstudent.cpp -o main