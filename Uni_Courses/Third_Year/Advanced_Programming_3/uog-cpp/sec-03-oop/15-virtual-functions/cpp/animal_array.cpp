#include <array>
#include <iostream>
#include <string_view>
#include <string>

class Animal
{
protected:
    std::string name;

    // We're making this constructor protected because
    // we don't want people creating Animal objects directly,
    // but we still want derived classes to be able to use it.
    Animal(std::string_view name)
        : name{ name }
    {
    }

    // To prevent slicing (covered later)
    Animal(const Animal&) = delete;
    Animal& operator=(const Animal&) = delete;

public:
    std::string_view get_name() const { return name; }
    std::string_view speak() const { return "WHAT?!?!"; }
};

class Cat: public Animal
{
public:
    Cat(std::string_view name) : Animal{ name } {}
    std::string_view speak() const { return "Meooow"; }
};

class Dog: public Animal
{
public: 
    Dog(std::string_view name) : Animal {name} {}
    std::string_view speak() const { return "Woooof"; }
};

class Tiger: public Animal
{
public:
    Tiger(std::string_view name) : Animal{ name } {}
    std::string_view speak() const { return "Roarrr"; }
};


int main()
{
    const Cat fred{ "Fred" };
    const Cat misty{ "Misty" };
    const Cat zeke{ "Zeke" };

    const Dog garbo{ "Garbo" };
    const Dog pooky{ "Pooky" };
    const Dog truffle{ "Truffle" };

    // Set up an array of pointers to animals, and set those pointers to our Cat and Dog objects
    const auto animals{ std::array<const Animal*, 6>({&fred, &garbo, &misty, 
                                                      &pooky, &truffle, &zeke }) };

    // Before C++20, with the array size being explicitly specified
    // const std::array<const Animal*, 6> animals{ &fred, &garbo, &misty, &pooky, &truffle, &zeke };

    for (const auto animal : animals) {
        std::cout << animal->get_name() << " says " << animal->speak() << '\n';
    }

    return 0;
}