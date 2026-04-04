#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../../concurrency/RideLock.h"
#include "../../engine/pricing/PricingEngine.h"
#include "../../engine/matching/MatchingEngine.h"
#include "../interfaces/IPricingStrategy.h"
#include "../models/Location.h"

namespace uber {

class SurgePricingStrategy;
class Rider;
class Driver;
class RiderService;
class DriverService;
class Ride;

class DispatchSystem : public std::enable_shared_from_this<DispatchSystem> {
public:
    DispatchSystem(std::shared_ptr<RiderService> riderService,
                   std::shared_ptr<DriverService> driverService,
                   std::shared_ptr<MatchingEngine> matchingEngine,
                   std::shared_ptr<PricingEngine> pricingEngine,
                   std::shared_ptr<IPricingStrategy> baselineStrategy,
                   std::shared_ptr<SurgePricingStrategy> surgeStrategy);

    void attachToRiderService();

    void registerRider(std::shared_ptr<Rider> rider);
    void registerDriver(std::shared_ptr<Driver> driver);

    std::shared_ptr<Ride> requestRide(const std::string& riderId,
                                      const Location& pickup,
                                      const Location& dropoff);

private:
    int countActiveRides() const;

    std::shared_ptr<RiderService> riderService_;
    std::shared_ptr<DriverService> driverService_;
    std::shared_ptr<MatchingEngine> matchingEngine_;
    std::shared_ptr<PricingEngine> pricingEngine_;
    std::shared_ptr<IPricingStrategy> baselineStrategy_;
    std::shared_ptr<SurgePricingStrategy> surgeStrategy_;

    mutable std::mutex dispatchMutex_;
    RideMutexRegistry rideMutexRegistry_;
    std::vector<std::shared_ptr<Ride>> trackedRides_;
};

}  // namespace uber
