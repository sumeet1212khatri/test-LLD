#include "MatchingEngine.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <sstream>

#include "../../core/models/Driver.h"
#include "../../core/models/Location.h"
#include "../../core/models/Ride.h"
#include "../../core/models/Rider.h"
#include "DriverSelector.h"

namespace uber {

namespace {

std::string makeRideId() {
    static std::atomic<std::uint64_t> counter{0};
    std::ostringstream oss;
    oss << "ride-" << ++counter;
    return oss.str();
}

}  // namespace

MatchingEngine::MatchingEngine() = default;

void MatchingEngine::registerDriver(std::shared_ptr<Driver> driver) {
    if (!driver) {
        return;
    }
    std::lock_guard<std::mutex> lock(driversMutex_);
    drivers_.push_back(std::move(driver));
}

std::vector<std::shared_ptr<Driver>> MatchingEngine::snapshotAvailableDriversLocked() {
    std::lock_guard<std::mutex> lock(driversMutex_);
    std::vector<std::shared_ptr<Driver>> available;
    available.reserve(drivers_.size());
    for (const auto& driver : drivers_) {
        if (driver && driver->isAvailable()) {
            available.push_back(driver);
        }
    }
    return available;
}

std::shared_ptr<Ride> MatchingEngine::requestRide(const std::shared_ptr<Rider>& rider,
                                                  const Location& pickup,
                                                  const Location& dropoff) {
    if (!rider) {
        return nullptr;
    }

    std::vector<std::shared_ptr<Driver>> candidates = snapshotAvailableDriversLocked();
    if (candidates.empty()) {
        return nullptr;
    }

    // IDriverSelector has no pickup parameter; strategies use Rider::location() for Haversine to drivers.
    Location savedRiderLocation = rider->location();
    rider->setLocation(pickup);

    PremiumDriverSelector premiumSelector;
    StandardDriverSelector standardSelector;
    IDriverSelector* selector = nullptr;
    if (rider->tier() == RiderTier::PREMIUM) {
        selector = &premiumSelector;
    } else {
        selector = &standardSelector;
    }

    std::shared_ptr<Driver> chosen = selector->selectDriver(rider, candidates);
    rider->setLocation(savedRiderLocation);

    if (!chosen) {
        return nullptr;
    }

    auto ride = std::make_shared<Ride>();
    ride->setId(makeRideId());
    ride->setRider(rider);
    ride->setDriver(chosen);
    ride->setPickup(pickup);
    ride->setDropoff(dropoff);
    ride->setStatus(RideStatus::REQUESTED);
    ride->setFare(0.0);
    ride->setRequestedAt(std::chrono::system_clock::now());
    return ride;
}

}  // namespace uber
