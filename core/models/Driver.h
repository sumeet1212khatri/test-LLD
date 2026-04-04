#pragma once

#include <memory>
#include <string>

#include "Location.h"

namespace uber {

class Ride;

class Driver {
public:
    Driver();
    Driver(std::string id,
           std::string name,
           float rating,
           Location location,
           bool isAvailable,
           std::shared_ptr<Ride> currentRide);

    const std::string& id() const;
    const std::string& name() const;
    float rating() const;
    const Location& location() const;
    bool isAvailable() const;
    const std::shared_ptr<Ride>& currentRide() const;

    void setId(std::string id);
    void setName(std::string name);
    void setRating(float rating);
    void setLocation(Location location);
    void setAvailable(bool available);
    void setCurrentRide(std::shared_ptr<Ride> ride);

private:
    std::string id_;
    std::string name_;
    float rating_;
    Location location_;
    bool isAvailable_;
    std::shared_ptr<Ride> currentRide_;
};

}  // namespace uber
