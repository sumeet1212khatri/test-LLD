#pragma once

#include "../../core/interfaces/IPricingStrategy.h"

namespace uber {

class BasePricingStrategy : public IPricingStrategy {
public:
    explicit BasePricingStrategy(double baseFare);
    ~BasePricingStrategy() override = default;

    double baseFare() const;
    void setBaseFare(double baseFare);

protected:
    double baseFare_;
};

}  // namespace uber
