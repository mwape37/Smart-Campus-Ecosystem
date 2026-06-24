#pragma once
#include "Resource.h"

class LabHardware : public Resource {
private:
    string warrantyDate;

public:
    LabHardware(int id, const string& name,
                double price, int stock,
                const string& warrantyDate);

    string getWarrantyDate() const;

    void   display()   const override;
    string serialize() const override;
};