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
// Cat and Dog from the example above

int main()
{
    // std::to_array is only after C++20
    // const auto& cats{ std::to_array<Cat>({{ "Fred" }, { "Misty" }, { "Zeke" }}) };
    // const auto& dogs{ std::to_array<Dog>({{ "Garbo" }, { "Pooky" }, { "Truffle" }}) };

    // Before C++20
    const std::array<Cat, 3> cats{ { { "Fred" }, { "Misty" }, { "Zeke" } } };
    const std::array<Dog, 3> dogs{ { { "Garbo" }, { "Pooky" }, { "Truffle" } } };

    for (const auto& cat : cats) {
        std::cout << cat.get_name() << " says " << cat.speak() << '\n';
    }

    for (const auto& dog : dogs) {
        std::cout << dog.get_name() << " says " << dog.speak() << '\n';
    }

    return 0;
}