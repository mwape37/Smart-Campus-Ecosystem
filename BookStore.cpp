#include "BookStore.h"

BookStore::BookStore(int id, const string& name,
                     double price, int stock,
                     const string& author)
    : Resource(id, name, "Bookstore Media", price, stock),
      author(author) {}

string BookStore::getAuthor() const { return author; }

// Prints base fields then author name
void BookStore::display() const {
    Resource::display();
    cout << "Author   : " << author << "\n";
}

// Appends author name to the base CSV line
string BookStore::serialize() const {
    return Resource::serialize() + "," + author;
}