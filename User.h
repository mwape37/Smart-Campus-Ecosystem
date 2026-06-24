#pragma once
#include <iostream>
#include <string>
using namespace std;

// Abstract base — holds shared identity data for all campus users
class User {
protected:
    string name;
    string campusID;

public:
    User(const string& name, const string& campusID);
    virtual ~User() = default;

    string getName()     const;
    string getCampusID() const;

    // Returns the price multiplier after discount (1.0 = no discount, 0.9 = 10% off)
    virtual double getDiscountMultiplier() const = 0;
    virtual string getRole()              const = 0;

    // Serializes user to a CSV line for file saving
    virtual string serialize() const;

    // Prints name, campus ID and discount rate
    virtual void printProfile() const;
};

// Student — no discount applied
class Student : public User {
public:
    Student(const string& name, const string& campusID);
    double getDiscountMultiplier() const override;
    string getRole()               const override;
};

// Staff — automatic 10% discount on every order
class Staff : public User {
    static constexpr double STAFF_DISCOUNT = 0.10;
public:
    Staff(const string& name, const string& campusID);
    double getDiscountMultiplier() const override;
    string getRole()               const override;
};