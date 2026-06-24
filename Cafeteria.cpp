#include "Cafeteria.h"

Cafeteria::Cafeteria(int id, const string& name,
                     double price, int stock,
                     const string& expiryDate)
    : Resource(id, name, "Cafeteria Perishables", price, stock),
      expiryDate(expiryDate) {}

string Cafeteria::getExpiryDate() const { return expiryDate; }

// Prints base fields then expiry date
void Cafeteria::display() const {
    Resource::display();
    cout << "Expires  : " << expiryDate << "\n";
}

// Appends expiry date to the base CSV line
string Cafeteria::serialize() const {
    return Resource::serialize() + "," + expiryDate;
}