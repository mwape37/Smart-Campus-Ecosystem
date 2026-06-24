#pragma once
#include <iostream>
#include <string>
#include <stdexcept>
#include <iomanip>
using namespace std;

// Abstract base — each payment type must validate itself and report its name
class Payment {
public:
    virtual ~Payment() = default;
    virtual void   validate()    const = 0;
    virtual string methodName()  const = 0;

    // Prints a confirmation line; subclasses may override if needed
    virtual void processPayment(double amount) const;
};

// Cash payment — always valid, no extra checks needed
class CashPayment : public Payment {
public:
    void   validate()   const override {}
    string methodName() const override { return "Cash"; }
};

// Card payment — card number must be exactly 16 numeric digits
class CardPayment : public Payment {
    string cardNumber;

    static bool isValidCard(const string& num);

public:
    explicit CardPayment(const string& cardNumber);
    void   validate()   const override;
    string methodName() const override;
};

// Validates then processes any Payment object
class PaymentProcessor {
public:
    static void process(Payment& payment, double amount);
};