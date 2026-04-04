#include "PerKmPricingStrategy.h"

namespace uber {

PerKmPricingStrategy::PerKmPricingStrategy(double baseFare, double ratePerKm)
    : BasePricingStrategy(baseFare), ratePerKm_(ratePerKm) {}

double PerKmPricingStrategy::ratePerKm() const {
    return ratePerKm_;
}

void PerKmPricingStrategy::setRatePerKm(double ratePerKm) {
    ratePerKm_ = ratePerKm;
}

double PerKmPricingStrategy::calculateFare(double distanceKm, int demandLevel) const {
    (void)demandLevel;
    return baseFare_ + (distanceKm * ratePerKm_);
}

}  // namespace uber
