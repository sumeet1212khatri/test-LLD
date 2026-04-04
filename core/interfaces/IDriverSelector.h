#pragma once

#include <memory>
#include <vector>

namespace uber {

class Rider;
class Driver;

class IDriverSelector {
public:
    virtual ~IDriverSelector() = default;

    virtual std::shared_ptr<Driver> selectDriver(
        const std::shared_ptr<Rider>& rider,
        const std::vector<std::shared_ptr<Driver>>& candidates) = 0;
};

}  // namespace uber
