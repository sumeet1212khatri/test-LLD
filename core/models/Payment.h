#pragma once

#include <string>

namespace uber {

enum class PaymentMethod { CASH, CARD, WALLET, UPI };

enum class PaymentStatus { PENDING, COMPLETED, FAILED, REFUNDED };

class Payment {
public:
    Payment();
    Payment(std::string rideId, double amount, PaymentMethod method, PaymentStatus status);

    const std::string& rideId() const;
    double amount() const;
    PaymentMethod method() const;
    PaymentStatus status() const;

    void setRideId(std::string rideId);
    void setAmount(double amount);
    void setMethod(PaymentMethod method);
    void setStatus(PaymentStatus status);

private:
    std::string rideId_;
    double amount_;
    PaymentMethod method_;
    PaymentStatus status_;
};

}  // namespace uber
