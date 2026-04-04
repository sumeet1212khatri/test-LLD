#include "BasePricingStrategy.h"

namespace uber {

BasePricingStrategy::BasePricingStrategy(double baseFare) : baseFare_(baseFare) {}

double BasePricingStrategy::baseFare() const {
    return baseFare_;
}

void BasePricingStrategy::setBaseFare(double baseFare) {
    baseFare_ = baseFare;
}

}  // namespace uber
