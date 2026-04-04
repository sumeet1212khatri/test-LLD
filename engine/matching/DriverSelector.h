#pragma once

#include <memory>
#include <vector>

#include "../../core/interfaces/IDriverSelector.h"

namespace uber {

class PremiumDriverSelector : public IDriverSelector {
public:
    ~PremiumDriverSelector() override = default;

    std::shared_ptr<Driver> selectDriver(
        const std::shared_ptr<Rider>& rider,
        const std::vector<std::shared_ptr<Driver>>& candidates) override;
};

class StandardDriverSelector : public IDriverSelector {
public:
    ~StandardDriverSelector() override = default;

    std::shared_ptr<Driver> selectDriver(
        const std::shared_ptr<Rider>& rider,
        const std::vector<std::shared_ptr<Driver>>& candidates) override;
};

}  // namespace uber
