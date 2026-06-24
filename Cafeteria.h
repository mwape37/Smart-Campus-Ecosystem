#pragma once
#include "Resource.h"

class Cafeteria : public Resource {
private:
    string expiryDate;

public:
    Cafeteria(int id, const string& name,
              double price, int stock,
              const string& expiryDate);

    string getExpiryDate() const;

    void   display()   const override;
    string serialize() const override;
};