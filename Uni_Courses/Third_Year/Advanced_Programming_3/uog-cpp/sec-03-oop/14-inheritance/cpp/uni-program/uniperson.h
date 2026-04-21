#ifndef UNIPERSON_H
#define UNIPERSON_H

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
    // forward declare everything here
    UniPerson(std::string_view name = "", int age = 0);
    const std::string& get_name() const;
    int get_age() const;
    int get_id() const;
};

#endif