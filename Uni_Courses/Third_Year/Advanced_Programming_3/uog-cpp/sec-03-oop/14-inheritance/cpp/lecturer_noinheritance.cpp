#include <string>
#include <string_view>

class Lecturer  // non inheritance from UniPerson
{
    static int nextID;
    double salary;
    std::string name;
    int age;
    int ID;
public:
    

    Lecturer(std::string name, int age): name{name}, age {age} 
    {
        // Assign the current counter and 
        // then increment it for the next object
        ID = nextID++;
    };
    // we must copy + paste what we wrote for UniPerson
    int age() { return age; }
    const std::string& name() { return name; }
};

// you immediately see the problem here
// we just count the ID from all over again.
int Lecturer::nextID{10000};    