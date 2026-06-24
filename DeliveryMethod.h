#pragma once
#include <string>
using namespace std;

// Abstract base — each delivery type defines its name and fee
class DeliveryMethod {
public:
    virtual ~DeliveryMethod() = default;
    virtual string methodName() const = 0;
    virtual double surcharge()  const = 0;
};

// Free pickup — no surcharge
class Pickup : public DeliveryMethod {
public:
    string methodName() const override { return "Pickup (Free)"; }
    double surcharge()  const override { return 0.0; }
};

// Dorm delivery — applies a flat fee
class DormDelivery : public DeliveryMethod {
    static constexpr double DELIVERY_FEE = 5.00;
public:
    string methodName() const override { return "Dorm Delivery"; }
    double surcharge()  const override { return DELIVERY_FEE; }
};