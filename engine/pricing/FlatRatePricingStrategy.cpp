#include "FlatRatePricingStrategy.h"

namespace uber {

FlatRatePricingStrategy::FlatRatePricingStrategy() : zoneRates_(), activeZone_() {}

FlatRatePricingStrategy::FlatRatePricingStrategy(std::unordered_map<std::string, double> zoneRates,
                                                 std::string activeZone)
    : zoneRates_(std::move(zoneRates)), activeZone_(std::move(activeZone)) {}

const std::string& FlatRatePricingStrategy::activeZone() const {
    return activeZone_;
}

void FlatRatePricingStrategy::setActiveZone(std::string zoneId) {
    activeZone_ = std::move(zoneId);
}

void FlatRatePricingStrategy::setZoneRate(const std::string& zoneId, double flatFare) {
    zoneRates_[zoneId] = flatFare;
}

double FlatRatePricingStrategy::getZoneRate(const std::string& zoneId) const {
    const auto it = zoneRates_.find(zoneId);
    if (it == zoneRates_.end()) {
        return 0.0;
    }
    return it->second;
}

double FlatRatePricingStrategy::calculateFare(double distanceKm, int demandLevel) const {
    (void)distanceKm;
    (void)demandLevel;
    const auto it = zoneRates_.find(activeZone_);
    if (it == zoneRates_.end()) {
        return 0.0;
    }
    return it->second;
}

}  // namespace uber
