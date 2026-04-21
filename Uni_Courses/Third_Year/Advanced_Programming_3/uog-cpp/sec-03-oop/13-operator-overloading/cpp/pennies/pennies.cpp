#include "pennies.h"

// note: this function is not a member function nor a friend function!
Pennies operator+(const Pennies& c1, const Pennies& c2)
{
  // use the Pennies constructor and operator+(int, int)
  // we don't need direct access to private members here
  return { c1.get_pennies() + c2.get_pennies() };
}