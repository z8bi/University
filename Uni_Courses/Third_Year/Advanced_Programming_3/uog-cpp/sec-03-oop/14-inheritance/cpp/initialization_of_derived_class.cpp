class Base
{
public:
    int id {};

    Base(int id=0) : id{ id }  {}

    int get_id() const { return id; }
};

class Derived: public Base
{
public:
    double value {};

    Derived(double value=0.0) : value{ value } {}

    double get_value() const { return value; }
};

int main()
{   
    // With non-derived classes, constructors only have to worry about
    // their own members.
    Base parent { 42 };   // use Base(int) constructor

    // How about the the "id" member that child inherits from Base?
    Derived child { 42.24 };  // use Derived(double) constructor

    return 0;
}