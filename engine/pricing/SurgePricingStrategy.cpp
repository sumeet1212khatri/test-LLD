#include "SurgePricingStrategy.h"

namespace uber {

SurgePricingStrategy::SurgePricingStrategy(double baseFare) : BasePricingStrategy(baseFare) {}

double SurgePricingStrategy::calculateFare(double distanceKm, int demandLevel) const {
    const double surgeMultiplier = 1.0 + (static_cast<double>(demandLevel) * 0.1);
    return baseFare_ + (distanceKm * 1.5 * surgeMultiplier);
}

}  // namespace uber
