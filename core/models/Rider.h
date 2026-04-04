#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Location.h"

namespace uber {

class Ride;

enum class RiderTier { PREMIUM, STANDARD };

class Rider {
public:
    Rider();
    Rider(std::string id, std::string name, RiderTier tier, Location location);

    const std::string& id() const;
    const std::string& name() const;
    RiderTier tier() const;
    const Location& location() const;
    const std::vector<std::shared_ptr<Ride>>& rideHistory() const;

    void setId(std::string id);
    void setName(std::string name);
    void setTier(RiderTier tier);
    void setLocation(Location location);

    void addRideToHistory(const std::shared_ptr<Ride>& ride);

private:
    std::string id_;
    std::string name_;
    RiderTier tier_;
    Location location_;
    std::vector<std::shared_ptr<Ride>> rideHistory_;
};

}  // namespace uber
