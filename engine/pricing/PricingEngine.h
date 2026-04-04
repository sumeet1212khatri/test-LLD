#pragma once

#include <memory>

#include "../../core/interfaces/IPricingStrategy.h"

namespace uber {

class PricingEngine {
public:
    PricingEngine();
    explicit PricingEngine(std::shared_ptr<IPricingStrategy> strategy);

    void setStrategy(std::shared_ptr<IPricingStrategy> strategy);
    const std::shared_ptr<IPricingStrategy>& strategy() const;

    double calculateFare(double distanceKm, int demandLevel) const;

private:
    std::shared_ptr<IPricingStrategy> strategy_;
};

}  // namespace uber
