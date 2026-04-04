#pragma once

#include <string>
#include <unordered_map>

#include "../../core/interfaces/IPricingStrategy.h"

namespace uber {

class FlatRatePricingStrategy : public IPricingStrategy {
public:
    FlatRatePricingStrategy();
    explicit FlatRatePricingStrategy(std::unordered_map<std::string, double> zoneRates,
                                     std::string activeZone);
    ~FlatRatePricingStrategy() override = default;

    const std::string& activeZone() const;
    void setActiveZone(std::string zoneId);

    void setZoneRate(const std::string& zoneId, double flatFare);
    double getZoneRate(const std::string& zoneId) const;

    double calculateFare(double distanceKm, int demandLevel) const override;

private:
    std::unordered_map<std::string, double> zoneRates_;
    std::string activeZone_;
};

}  // namespace uber
