#include "RiderService.h"

#include "DispatchSystem.h"
#include "../models/Location.h"
#include "../models/Ride.h"
#include "../models/Rider.h"

namespace uber {

RiderService::RiderService() = default;

void RiderService::setDispatch(std::weak_ptr<DispatchSystem> dispatch) {
    dispatch_ = std::move(dispatch);
}

void RiderService::registerRider(std::shared_ptr<Rider> rider) {
    if (!rider) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    riders_[rider->id()] = std::move(rider);
}

std::shared_ptr<Ride> RiderService::requestRide(const std::string& riderId,
                                                const Location& pickup,
                                                const Location& dropoff) {
    const auto dispatch = dispatch_.lock();
    if (!dispatch) {
        return nullptr;
    }
    return dispatch->requestRide(riderId, pickup, dropoff);
}

std::vector<std::shared_ptr<Ride>> RiderService::getRideHistory(const std::string& riderId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = riders_.find(riderId);
    if (it == riders_.end() || !it->second) {
        return {};
    }
    return it->second->rideHistory();
}

std::shared_ptr<Rider> RiderService::findRider(const std::string& riderId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = riders_.find(riderId);
    if (it == riders_.end()) {
        return nullptr;
    }
    return it->second;
}

}  // namespace uber
