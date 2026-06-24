#include "LabHardware.h"

LabHardware::LabHardware(int id, const string& name,
                         double price, int stock,
                         const string& warrantyDate)
    : Resource(id, name, "Lab Hardware", price, stock),
      warrantyDate(warrantyDate) {}

string LabHardware::getWarrantyDate() const { return warrantyDate; }

// Prints base fields then warranty date
void LabHardware::display() const {
    Resource::display();
    cout << "Warranty : " << warrantyDate << "\n";
}

// Appends warranty date to the base CSV line
string LabHardware::serialize() const {
    return Resource::serialize() + "," + warrantyDate;
}