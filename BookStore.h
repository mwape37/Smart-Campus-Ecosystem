#pragma once
#include "Resource.h"

class BookStore : public Resource {
private:
    string author;

public:
    BookStore(int id, const string& name,
              double price, int stock,
              const string& author);

    string getAuthor() const;

    void   display()   const override;
    string serialize() const override;
};