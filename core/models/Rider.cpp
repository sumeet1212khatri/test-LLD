#include "Rider.h"

#include "Ride.h"

namespace uber {

Rider::Rider() : id_(), name_(), tier_(RiderTier::STANDARD), location_(), rideHistory_() {}

Rider::Rider(std::string id, std::string name, RiderTier tier, Location location)
    : id_(std::move(id)),
      name_(std::move(name)),
      tier_(tier),
      location_(std::move(location)),
      rideHistory_() {}

const std::string& Rider::id() const {
    return id_;
}

const std::string& Rider::name() const {
    return name_;
}

RiderTier Rider::tier() const {
    return tier_;
}

const Location& Rider::location() const {
    return location_;
}

const std::vector<std::shared_ptr<Ride>>& Rider::rideHistory() const {
    return rideHistory_;
}

void Rider::setId(std::string id) {
    id_ = std::move(id);
}

void Rider::setName(std::string name) {
    name_ = std::move(name);
}

void Rider::setTier(RiderTier tier) {
    tier_ = tier;
}

void Rider::setLocation(Location location) {
    location_ = std::move(location);
}

void Rider::addRideToHistory(const std::shared_ptr<Ride>& ride) {
    if (ride) {
        rideHistory_.push_back(ride);
    }
}

}  // namespace uber
