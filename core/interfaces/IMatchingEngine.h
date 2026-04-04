#pragma once

#include <memory>

namespace uber {

class Rider;
class Ride;
class Location;

class IMatchingEngine {
public:
    virtual ~IMatchingEngine() = default;

    virtual std::shared_ptr<Ride> requestRide(const std::shared_ptr<Rider>& rider,
                                              const Location& pickup,
                                              const Location& dropoff) = 0;
};

}  // namespace uber
