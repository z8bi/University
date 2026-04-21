#include <iostream>
#include <string_view>
#include <string>

class Animal
{
protected:
    std::string name;

    Animal(std::string_view name) : name{ name } { }

    // To prevent slicing (covered later)
    Animal(const Animal&) = delete;
    Animal& operator=(const Animal&) = delete;

public:
    std::string_view get_name() const { return name; }
    virtual std::string_view speak() const { return "WHAT?!?!"; }
};

class Cat: public Animal
{
public:
    Cat(std::string_view name) : Animal{ name } {}
    virtual std::string_view speak() const { return "Meooow"; }
};

class Tiger: public Animal
{
public:
    Tiger(std::string_view name) : Animal{ name } {}
    virtual std::string_view speak() const { return "Roarrr"; }
};

void report(const Animal& animal) {
    std::cout << animal.get_name() << " says " << animal.speak() << '\n';
}

int main()
{
    Cat cat{ "khiem" };
    Tiger tiger{ "zing" };
    report(cat);
    report(tiger);

    // Let us make an array and loop through it
    Cat fred { "fred" };
    Cat misty { "misty" };
    Cat zeke { "zeke" };
    Tiger garbo { "garbo" };
    Tiger pooky { "pooky" };
    Tiger truffle { "truffle" };

    Animal* aniamls[] {&fred, &garbo, &misty, &pooky, &zeke, &truffle};
    for (const auto* animal: aniamls) {
        std::cout << animal->get_name() << " says " << animal->speak() << '\n';
    }
    return 0;
}