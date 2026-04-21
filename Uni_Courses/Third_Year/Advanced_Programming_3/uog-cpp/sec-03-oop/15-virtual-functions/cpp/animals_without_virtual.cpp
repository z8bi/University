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
        : name{ name } { }

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

class Tiger: public Animal
{
public:
    Tiger(std::string_view name) : Animal{ name } {}
    std::string_view speak() const { return "Roarrr"; }
};

void report(const Animal& animal) {
    std::cout << animal.get_name() << " says " << animal.speak() << '\n';
}

int main()
{
    const Cat cat { "khiem" };
    const Tiger tiger{ "zing" };
    report(cat);
    report(tiger);

    return 0;
}