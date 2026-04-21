#include "uniperson.h"

int UniPerson::nextID{100000}; // Static initialization here only!

UniPerson::UniPerson(std::string_view n, int a) 
: name{n}, age{a}, ID{nextID++} 
{}
const std::string& UniPerson::get_name() const {
    return name; 
}
int UniPerson::get_age() const { 
    return age; 
}
int UniPerson::get_id() const {
    return ID; 
}