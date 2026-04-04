#include "DriverSelector.h"

#include <limits>

#include "../../core/models/Driver.h"
#include "../../core/models/Location.h"
#include "../../core/models/Rider.h"

namespace uber {

namespace {

double distanceFromPickupToDriver(const Location& pickup, const std::shared_ptr<Driver>& driver) {
    if (!driver) {
        return std::numeric_limits<double>::max();
    }
    return pickup.distanceTo(driver->location());
}

std::shared_ptr<Driver> selectNearestFrom(const Location& pickup,
                                            const std::vector<std::shared_ptr<Driver>>& candidates) {
    std::shared_ptr<Driver> best;
    double bestDistance = std::numeric_limits<double>::max();
    for (const auto& driver : candidates) {
        if (!driver || !driver->isAvailable()) {
            continue;
        }
        const double distance = distanceFromPickupToDriver(pickup, driver);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = driver;
        }
    }
    return best;
}

}  // namespace

std::shared_ptr<Driver> PremiumDriverSelector::selectDriver(
    const std::shared_ptr<Rider>& rider,
    const std::vector<std::shared_ptr<Driver>>& candidates) {
    std::vector<std::shared_ptr<Driver>> filtered;
    filtered.reserve(candidates.size());
    for (const auto& driver : candidates) {
        if (driver && driver->isAvailable() && driver->rating() >= 4.5F) {
            filtered.push_back(driver);
        }
    }
    if (filtered.empty()) {
        return nullptr;
    }
    const Location pickup = rider ? rider->location() : Location();
    return selectNearestFrom(pickup, filtered);
}

std::shared_ptr<Driver> StandardDriverSelector::selectDriver(
    const std::shared_ptr<Rider>& rider,
    const std::vector<std::shared_ptr<Driver>>& candidates) {
    const Location pickup = rider ? rider->location() : Location();
    return selectNearestFrom(pickup, candidates);
}

}  // namespace uber
