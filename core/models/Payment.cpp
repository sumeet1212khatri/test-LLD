#include "Payment.h"

namespace uber {

Payment::Payment()
    : rideId_(), amount_(0.0), method_(PaymentMethod::CASH), status_(PaymentStatus::PENDING) {}

Payment::Payment(std::string rideId, double amount, PaymentMethod method, PaymentStatus status)
    : rideId_(std::move(rideId)), amount_(amount), method_(method), status_(status) {}

const std::string& Payment::rideId() const {
    return rideId_;
}

double Payment::amount() const {
    return amount_;
}

PaymentMethod Payment::method() const {
    return method_;
}

PaymentStatus Payment::status() const {
    return status_;
}

void Payment::setRideId(std::string rideId) {
    rideId_ = std::move(rideId);
}

void Payment::setAmount(double amount) {
    amount_ = amount;
}

void Payment::setMethod(PaymentMethod method) {
    method_ = method;
}

void Payment::setStatus(PaymentStatus status) {
    status_ = status;
}

}  // namespace uber
