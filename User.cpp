#include "User.h"

User::User(const string& name, const string& campusID)
    : name(name), campusID(campusID) {}

string User::getName()     const { return name; }
string User::getCampusID() const { return campusID; }

// Returns campusID,name,role as a CSV line
string User::serialize() const {
    return campusID + "," + name + "," + getRole();
}

void User::printProfile() const {
    double discountPct = (1.0 - getDiscountMultiplier()) * 100.0;
    cout << "[" << getRole() << "]"
         << "  Name: "         << name
         << "  |  Campus ID: " << campusID
         << "  |  Discount: "  << discountPct << "%\n";
}

Student::Student(const string& name, const string& campusID)
    : User(name, campusID) {}

double Student::getDiscountMultiplier() const { return 1.0; }
string Student::getRole()               const { return "Student"; }

Staff::Staff(const string& name, const string& campusID)
    : User(name, campusID) {}

double Staff::getDiscountMultiplier() const { return 1.0 - STAFF_DISCOUNT; }
string Staff::getRole()               const { return "Staff"; }