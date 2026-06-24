#include "Order.h"

int Order::nextOrderId = 0;

Order::Order(User* user, DeliveryMethod* delivery)
    : orderId(++nextOrderId), user(user),
      delivery(delivery), finalized(false) {}

int Order::getOrderId() const { return orderId; }

void Order::addItem(Resource* resource, int qty) {
    if (finalized)
        throw logic_error("Cannot modify a finalized order.");
    items.push_back({resource, qty});
}

double Order::rawSubtotal() const {
    double total = 0.0;
    for (const auto& item : items)
        total += item.subtotal();
    return total;
}

double Order::finalTotal() const {
    return rawSubtotal() * user->getDiscountMultiplier()
           + delivery->surcharge();
}

void Order::checkout() {
    if (items.empty())
        throw logic_error("Order is empty. Add items before checking out.");

    // Validate all items first — all-or-nothing, no partial deductions
    for (const auto& item : items)
        if (item.resource->getStock() < item.quantity)
            throw runtime_error(
                "Insufficient stock for '" + item.resource->getName() +
                "'. Available: " + to_string(item.resource->getStock()) +
                ", Requested: "  + to_string(item.quantity));

    // Deduct stock only after every item passes validation
    for (auto& item : items)
        item.resource->purchase(item.quantity);

    finalized = true;
    printReceipt();
}

void Order::printReceipt() const {
    double sub      = rawSubtotal();
    double discount = sub - sub * user->getDiscountMultiplier();
    double total    = finalTotal();

    cout << "\n========== ORDER RECEIPT #" << orderId << " ==========\n";
    user->printProfile();
    cout << "Delivery : " << delivery->methodName()
         << " (EGP " << fixed << setprecision(2) << delivery->surcharge() << ")\n";
    cout << string(43, '-') << "\n";
    cout << left << setw(20) << "Resource"
                 << setw(8)  << "Qty"
                 << "Subtotal\n";
    for (const auto& item : items)
        cout << setw(20) << item.resource->getName()
             << setw(8)  << item.quantity
             << "EGP " << fixed << setprecision(2) << item.subtotal() << "\n";
    cout << string(43, '-') << "\n";
    cout << "Raw Subtotal : EGP " << fixed << setprecision(2) << sub     << "\n";
    if (discount > 0.0)
        cout << "Discount     : -EGP " << setprecision(2) << discount    << "\n";
    cout << "Delivery Fee : +EGP " << setprecision(2) << delivery->surcharge() << "\n";
    cout << "TOTAL        :  EGP " << setprecision(2) << total           << "\n";
    cout << "===========================================\n\n";
}