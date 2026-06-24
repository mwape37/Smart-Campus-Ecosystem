#include "Payment.h"

void Payment::processPayment(double amount) const {
    cout << "[Payment] " << methodName()
         << " | Amount: EGP " << fixed << setprecision(2) << amount
         << " — processed successfully.\n";
}

// Returns true only if num is exactly 16 digits with no other characters
bool CardPayment::isValidCard(const string& num) {
    if (num.size() != 16) return false;
    for (char c : num)
        if (!isdigit(c)) return false;
    return true;
}

CardPayment::CardPayment(const string& cardNumber)
    : cardNumber(cardNumber) {}

void CardPayment::validate() const {
    if (!isValidCard(cardNumber))
        throw invalid_argument(
            "Invalid card number '" + cardNumber +
            "'. Must be exactly 16 numeric digits.");
}

string CardPayment::methodName() const {
    return "Card (" + cardNumber + ")";
}

// Validates the payment method then processes it; throws on failure
void PaymentProcessor::process(Payment& payment, double amount) {
    payment.validate();
    payment.processPayment(amount);
}