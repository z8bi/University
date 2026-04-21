#include <iostream>
#include <vector>
#include <string>
#include <functional> // For std::reference_wrapper

class Teacher {
private:
    std::string name;
public:
    Teacher(std::string name) : name(name) {}
    std::string getName() const { return name; }
};

class Department {
private:
    std::string name;
    // Aggregation: We store references to teachers who exist elsewhere
    std::vector<std::reference_wrapper<Teacher>> teachers;

public:
    Department(std::string name) : name(name) {}

    void addTeacher(Teacher& t) {
        teachers.push_back(t);
    }

    void showRoster() const {
        std::cout << "Department: " << name << "\n";
        std::cout << "Staff List:\n";
        for (const auto& t : teachers) {
            std::cout << " - " << t.get().getName() << "\n";
        }
    }
};

int main() {
    // 1. Create the 'Teachers' (The Avengers)
    // These objects live in main's scope, independent of the department
    std::vector<Teacher> avengers = {
        Teacher("Tony Stark"),
        Teacher("Steve Rogers"),
        Teacher("Natasha Romanoff"),
        Teacher("Bruce Banner"),
        Teacher("Thor Odinson"),
        Teacher("Clint Barton"),
        Teacher("Wanda Maximoff"),
        Teacher("Vision"),
        Teacher("Sam Wilson"),
        Teacher("James Rhodes")
    };

    // 2. Create the Department
    Department heroDept("Tactical Operations");

    // 3. Add them to the department
    for (auto& member : avengers) {
        heroDept.addTeacher(member);
    }

    // 4. Display the results
    heroDept.showRoster();

    return 0;
}