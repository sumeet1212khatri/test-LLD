#pragma once

#include "BasePricingStrategy.h"

namespace uber {

class PerKmPricingStrategy : public BasePricingStrategy {
public:
    PerKmPricingStrategy(double baseFare, double ratePerKm);
    ~PerKmPricingStrategy() override = default;

    double ratePerKm() const;
    void setRatePerKm(double ratePerKm);

    double calculateFare(double distanceKm, int demandLevel) const override;

private:
    double ratePerKm_;
};

}  // namespace uber
