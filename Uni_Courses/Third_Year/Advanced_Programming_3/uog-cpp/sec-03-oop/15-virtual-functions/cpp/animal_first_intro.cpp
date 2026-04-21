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
    const Cat cat { "khiem" };
    std::cout << "cat is named " << cat.get_name() 
              << ", and it says " << cat.speak() << '\n';

    const Tiger tiger{ "zing" };
    std::cout << "tiger is named " << tiger.get_name() 
              << ", and it says " << tiger.speak() << '\n';

    const Dog dog {"dieu"};
    std::cout << "dog is named " << dog.get_name() 
              << ", and it says" << dog.speak() << '\n';

    const Animal* animal_ptr { &cat };
    std::cout << "animal_ptr is named " << animal_ptr->get_name() 
              << ", and it says " << animal_ptr->speak() << '\n';

    animal_ptr = &tiger;
    std::cout << "animal_ptr is named " << animal_ptr->get_name() 
              << ", and it says " << animal_ptr->speak() << '\n';

    return 0;
}