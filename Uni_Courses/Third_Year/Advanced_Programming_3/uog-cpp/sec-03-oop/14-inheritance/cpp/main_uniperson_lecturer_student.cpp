#include <string>
#include <string_view>
#include <iostream>
class UniPerson
{
protected:  // Protected so that Lecturer can see these
    // This is shared by all Person objects
    static int nextID;
    std::string name{};
    int ID {};
    int age {};
public:
    UniPerson(std::string_view name = "", int age = 0)
        : name{ name }, age{ age }, ID {nextID++}
    {
    }

    const std::string& get_name() const { return name; }
    int get_age() const { return age; }
    int get_id() const { return ID; }
    
};

int UniPerson::nextID{100000};

class Lecturer : public UniPerson  // non inheritance from UniPerson
{
private:
    double salary;
    // We can remove a lot of code here!!!
public:
    

    Lecturer(std::string_view name = "", int age = 0, double salary = 42000): 
        UniPerson{name, age}, salary {salary}
    {
        // we borrowed the constructor from UniPerson above
    };
    double get_salary() const { return salary; }
    // We can remove a lot of code for member functions here.
};

class Student : public UniPerson  // non inheritance from UniPerson
{
private:
    double tuition;
    // We can remove a lot of code here!!!
public:
    Student(std::string_view name = "", int age = 0, double salary = 42000): 
        UniPerson{name, age}, tuition {tuition}
    {
        // we borrowed the constructor from UniPerson above
    };
    double get_tuition() const { return tuition; }
    // We can remove a lot of code for member functions here.
};

// We don't need this code anymore and it makes perfect sense.
// int Lecturer::nextID{10000};    
int main()
{
    Lecturer khiem {"khiem", 42};
    std::cout << "Khiem's ID is " << khiem.get_id() << '\n';
    std::cout << "Khiem's salary is " << khiem.get_salary() << '\n';
    
    Student peter_parker {"peter", 18}; // his hair tells me that
    std::cout << "Peter's ID is " << peter_parker.get_id() << '\n';
    std::cout << "Peter's tuition fee is " << peter_parker.get_tuition() << '\n';
}
