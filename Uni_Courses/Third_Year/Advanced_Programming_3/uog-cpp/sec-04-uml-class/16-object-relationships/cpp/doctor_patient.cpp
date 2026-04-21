#include <functional> // reference_wrapper
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

// Since Doctor and Patient have a circular dependency, we're going to forward declare Patient
class Patient;

class Doctor
{
private:
	std::string name{};
	std::vector<std::reference_wrapper<const Patient>> patients{};

public:
	Doctor(std::string_view name) :
		name{ name }
	{
	}

	void add_patient(Patient& patient);

	// We'll implement this function below Patient since we need Patient to be defined at that point
	friend std::ostream& operator<<(std::ostream& out, const Doctor& doctor);

	const std::string& get_name() const { return name; }
};

class Patient
{
private:
	std::string name{};
	std::vector<std::reference_wrapper<const Doctor>> doctors{}; // so that we can use it here

	// We're going to make add_doctor private because we don't want the public to use it.
	// They should use Doctor::add_patient() instead, which is publicly exposed
	void add_doctor(const Doctor& doctor)
	{
		doctors.push_back(doctor);
	}

public:
	Patient(std::string_view name)
		: name{ name }
	{
	}

	// We'll implement this function below to parallel operator<<(std::ostream&, const Doctor&)
	friend std::ostream& operator<<(std::ostream& out, const Patient& patient);

	const std::string& get_name() const { return name; }

	// We'll friend Doctor::add_patient() so it can access the private function Patient::add_doctor()
	friend void Doctor::add_patient(Patient& patient);
};

void Doctor::add_patient(Patient& patient)
{
	// Our doctor will add this patient
	patients.push_back(patient);

	// and the patient will also add this doctor
	patient.add_doctor(*this);
}

std::ostream& operator<<(std::ostream& out, const Doctor& doctor)
{
	if (doctor.patients.empty())
	{
		out << doctor.name << " has no patients right now";
		return out;
	}

	out << doctor.name << " is seeing patients: ";
	for (const auto& patient : doctor.patients)
		out << patient.get().get_name() << ' ';

	return out;
}

std::ostream& operator<<(std::ostream& out, const Patient& patient)
{
	if (patient.doctors.empty())
	{
		out << patient.get_name() << " has no doctors right now";
		return out;
	}

	out << patient.name << " is seeing doctors: ";
	for (const auto& doctor : patient.doctors)
		out << doctor.get().get_name() << ' ';

	return out;
}

int main()
{
	// Create a Patient outside the scope of the Doctor
	Patient dave{ "Dave" };
	Patient frank{ "Frank" };
	Patient betsy{ "Betsy" };

	Doctor james{ "James" };
	Doctor scott{ "Scott" };

	james.add_patient(dave);

	scott.add_patient(dave);
	scott.add_patient(betsy);

	std::cout << james << '\n';
	std::cout << scott << '\n';
	std::cout << dave << '\n';
	std::cout << frank << '\n';
	std::cout << betsy << '\n';

	return 0;
}