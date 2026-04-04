#pragma once

#include "BasePricingStrategy.h"

namespace uber {

class SurgePricingStrategy : public BasePricingStrategy {
public:
    explicit SurgePricingStrategy(double baseFare);
    ~SurgePricingStrategy() override = default;

    double calculateFare(double distanceKm, int demandLevel) const override;
};

}  // namespace uber
