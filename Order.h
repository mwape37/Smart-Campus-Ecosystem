#pragma once
#include <iostream>
#include <vector>
#include <iomanip>
#include <stdexcept>
#include "Resource.h"
#include "User.h"
#include "DeliveryMethod.h"
using namespace std;

// Pairs a resource with the quantity being ordered
struct OrderItem {
    Resource* resource;
    int       quantity;
    double subtotal() const { return resource->getPrice() * quantity; }
};

// Aggregates items for one user; applies discount and delivery fee on checkout
class Order {
    static int nextOrderId;

    int               orderId;
    User*             user;
    vector<OrderItem> items;
    DeliveryMethod*   delivery;
    bool              finalized;

    // Prints receipt — only called internally after a successful checkout
    void printReceipt() const;

public:
    Order(User* user, DeliveryMethod* delivery);

    int getOrderId() const;

    // Adds a resource + quantity; blocked if order is already finalized
    void addItem(Resource* resource, int qty);

    // Sum of all item subtotals before discount or delivery fee
    double rawSubtotal() const;

    // Final price after applying user discount and delivery surcharge
    double finalTotal() const;

    // Validates stock, deducts inventory, finalizes and prints receipt
    void checkout();
};