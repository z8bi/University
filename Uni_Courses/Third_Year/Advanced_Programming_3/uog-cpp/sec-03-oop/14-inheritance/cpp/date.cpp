class Date
{
    int day { 1 };
    int month { 1 };
    int year { 2000 };

    Date(int day, int month, int year) : day {day}, month {month}, year{year} 
    {
        // Test whether the date is in the right format.
    }
};