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
    // uncomment the constructor to see error
    // Derived(double value=0.0, int id=0) : value{ value }, id {id} {}

    double get_value() const { return value; }
};

int main()
{   
    // With non-derived classes, constructors only have to worry about
    // their own members.
    Base base { 42 };   // use Base(int) constructor

    // How about the the "id" member that derived inherits from Base?
    Derived derived { 42.24 };  // use Derived(double) constructor

    return 0;
}