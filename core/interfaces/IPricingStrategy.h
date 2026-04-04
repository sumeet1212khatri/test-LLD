#pragma once

namespace uber {

class IPricingStrategy {
public:
    virtual ~IPricingStrategy() = default;

    virtual double calculateFare(double distanceKm, int demandLevel) const = 0;
};

}  // namespace uber
