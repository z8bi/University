#ifndef PENNIES_H
#define PENNIES_H

class Pennies
{
private:
  int pennies{};

public:
  Pennies(int pennies)
    : pennies{ pennies }
  {}

  int get_pennies() const { return pennies; }
};

// Need to explicitly provide prototype for operator+ so uses of operator+ in other files know this overload exists
Pennies operator+(const Pennies& c1, const Pennies& c2);

#endif