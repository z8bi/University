class Base
{
public:
    int public_mem {}; // can be accessed by anybody
protected:
    int protected_mem {}; // can be accessed by Base members, friends, and derived classes
private:
    int private_mem {}; // can only be accessed by Base members and friends (but not derived classes)
};

class Derived: public Base
{
public:
    Derived()
    {
        public_mem = 1; // allowed: can access public base members from derived class
        protected_mem = 2; // allowed: can access protected base members from derived class
        
        // private_mem = 3; // not allowed: can not access private base members from derived class
    }
};

int main()
{
    Base base;
    base.public_mem = 1; // allowed: can access public members from outside class

    // base.protected_mem = 2; // not allowed: can not access protected members from outside class
    // base.private_mem = 3; // not allowed: can not access private members from outside class

    return 0;
}