#include <string>
#include <string_view>

class UniPerson
{
private:
    // This is shared by all Person objects
    static int nextID;
    std::string name{};
    int ID {};
    int age {};
public:
    UniPerson(std::string_view name = "", int age = 0)
        : name{ name }, age{ age }
    {
        // Assign the current counter and 
        // then increment it for the next object
        ID = nextID++;
    }

    const std::string& get_name() const { return name; }
    int get_age() const { return age; }
    int get_id() const { return ID; }
};

int UniPerson::nextID{100000};