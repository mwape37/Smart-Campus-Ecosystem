#include "Resource.h"

Resource::Resource()
    : id(0), name(""), category(""), price(0.0), stock(0) {}

Resource::Resource(int id, const string& name, const string& category,
                   double price, int stock)
    : id(id), name(name), category(category), price(price), stock(stock) {
    if (price < 0) throw invalid_argument("Price cannot be negative.");
    if (stock < 0) throw invalid_argument("Stock cannot be negative.");
}

int    Resource::getId()       const { return id; }
string Resource::getName()     const { return name; }
string Resource::getCategory() const { return category; }
double Resource::getPrice()    const { return price; }
int    Resource::getStock()    const { return stock; }

// Increases stock by qty
void Resource::restock(int qty) {
    if (qty <= 0) throw invalid_argument("Restock quantity must be positive.");
    stock += qty;
    cout << "[Restock] " << name
         << " | Added: " << qty
         << " | New stock: " << stock << "\n";
}

// Decreases stock by qty; throws and blocks if stock is insufficient
void Resource::purchase(int qty) {
    if (qty <= 0) throw invalid_argument("Purchase quantity must be positive.");
    if (qty > stock)
        throw runtime_error(
            "Insufficient stock for '" + name + "'. "
            "Available: " + to_string(stock) +
            ", Requested: " + to_string(qty));
    stock -= qty;
    cout << "[Purchase] " << name
         << " | Sold: " << qty
         << " | Remaining: " << stock << "\n";
}

// Returns base fields as a comma-separated string
string Resource::serialize() const {
    return to_string(id)    + "," +
           name             + "," +
           category         + "," +
           to_string(price) + "," +
           to_string(stock);
}

// Prints all base fields
void Resource::display() const {
    cout << "ID       : " << id       << "\n";
    cout << "Name     : " << name     << "\n";
    cout << "Category : " << category << "\n";
    cout << "Price    : EGP " << fixed << setprecision(2) << price << "\n";
    cout << "Stock    : " << stock    << " units\n";
}

bool Resource::operator>(const Resource& other)  const { return price >  other.price; }
bool Resource::operator<(const Resource& other)  const { return price <  other.price; }
bool Resource::operator==(const Resource& other) const { return price == other.price; }

void compareCost(const Resource& a, const Resource& b) {
    cout << "\n[Cost Comparison]\n";
    if (a > b)
        cout << a.getName() << " (EGP " << fixed << setprecision(2) << a.getPrice()
             << ") has a higher cost than "
             << b.getName() << " (EGP " << b.getPrice() << ")\n";
    else if (b > a)
        cout << b.getName() << " (EGP " << fixed << setprecision(2) << b.getPrice()
             << ") has a higher cost than "
             << a.getName() << " (EGP " << a.getPrice() << ")\n";
    else
        cout << "Both resources have equal cost (EGP "
             << fixed << setprecision(2) << a.getPrice() << ")\n";
}