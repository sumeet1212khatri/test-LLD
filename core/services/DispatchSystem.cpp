#include "DispatchSystem.h"

#include "../../engine/pricing/SurgePricingStrategy.h"
#include "../../engine/state/RideStateMachine.h"
#include "../../concurrency/RideLock.h"
#include "../models/Driver.h"
#include "../models/Ride.h"
#include "../models/Rider.h"
#include "DriverService.h"
#include "RiderService.h"

namespace uber {

DispatchSystem::DispatchSystem(std::shared_ptr<RiderService> riderService,
                               std::shared_ptr<DriverService> driverService,
                               std::shared_ptr<MatchingEngine> matchingEngine,
                               std::shared_ptr<PricingEngine> pricingEngine,
                               std::shared_ptr<IPricingStrategy> baselineStrategy,
                               std::shared_ptr<SurgePricingStrategy> surgeStrategy)
    : riderService_(std::move(riderService)),
      driverService_(std::move(driverService)),
      matchingEngine_(std::move(matchingEngine)),
      pricingEngine_(std::move(pricingEngine)),
      baselineStrategy_(std::move(baselineStrategy)),
      surgeStrategy_(std::move(surgeStrategy)) {}

void DispatchSystem::attachToRiderService() {
    riderService_->setDispatch(shared_from_this());
}

void DispatchSystem::registerRider(std::shared_ptr<Rider> rider) {
    riderService_->registerRider(std::move(rider));
}

void DispatchSystem::registerDriver(std::shared_ptr<Driver> driver) {
    if (!driver) {
        return;
    }
    std::lock_guard<std::mutex> lock(dispatchMutex_);
    driverService_->registerDriver(driver);
    matchingEngine_->registerDriver(std::move(driver));
}

int DispatchSystem::countActiveRides() const {
    int count = 0;
    for (const auto& ride : trackedRides_) {
        if (!ride) {
            continue;
        }
        const RideStatus status = ride->status();
        if (status == RideStatus::ASSIGNED || status == RideStatus::IN_PROGRESS) {
            ++count;
        }
    }
    return count;
}

std::shared_ptr<Ride> DispatchSystem::requestRide(const std::string& riderId,
                                                  const Location& pickup,
                                                  const Location& dropoff) {
    std::lock_guard<std::mutex> lock(dispatchMutex_);

    const auto rider = riderService_->findRider(riderId);
    if (!rider) {
        return nullptr;
    }

    const double distanceKm = pickup.distanceTo(dropoff);

    const int activeRides = countActiveRides();
    const int availableDrivers = driverService_->countAvailableDrivers();

    const bool surge =
        static_cast<double>(activeRides) > static_cast<double>(availableDrivers) * 0.7;
    if (surge) {
        pricingEngine_->setStrategy(surgeStrategy_);
    } else {
        pricingEngine_->setStrategy(baselineStrategy_);
    }

    const double fare = pricingEngine_->calculateFare(distanceKm, activeRides);

    auto ride = matchingEngine_->requestRide(rider, pickup, dropoff);
    if (!ride) {
        return nullptr;
    }

    const std::shared_ptr<std::mutex> rideMutex = rideMutexRegistry_.mutexFor(ride->id());
    RideLock rideLock(rideMutex);

    ride->setFare(fare);
    RideStateMachine::transition(ride, RideStatus::ASSIGNED);

    const auto driver = ride->driver();
    if (!driver) {
        return nullptr;
    }

    driver->setCurrentRide(ride);
    driver->setAvailable(false);

    rider->addRideToHistory(ride);
    trackedRides_.push_back(ride);

    return ride;
}

}  // namespace uber
