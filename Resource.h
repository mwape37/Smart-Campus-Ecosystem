#pragma once
#include <iostream>
#include <string>
#include <stdexcept>
#include <iomanip>
using namespace std;

class Resource {
protected:
    int    id;
    string name;
    string category;
    double price;
    int    stock;

public:
    Resource();
    Resource(int id, const string& name, const string& category,
             double price, int stock);
    virtual ~Resource() = default;

    int    getId()       const;
    string getName()     const;
    string getCategory() const;
    double getPrice()    const;
    int    getStock()    const;

    // Add or remove stock; throws on invalid input or insufficient stock
    void restock(int qty);
    void purchase(int qty);

    // Converts resource data to a CSV line for file saving
    virtual string serialize() const;

    // Prints all resource fields to console
    virtual void display() const;

    // Compare resources by price
    bool operator>(const Resource& other)  const;
    bool operator<(const Resource& other)  const;
    bool operator==(const Resource& other) const;
};

// Prints which resource costs more, or if they are equal
void compareCost(const Resource& a, const Resource& b);