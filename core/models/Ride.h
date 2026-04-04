#pragma once

#include <chrono>
#include <memory>
#include <string>

#include "Location.h"

namespace uber {

class Rider;
class Driver;

enum class RideStatus {
    REQUESTED,
    ASSIGNED,
    IN_PROGRESS,
    COMPLETED,
    CANCELLED
};

class Ride {
public:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

    Ride();
    Ride(std::string id,
         std::shared_ptr<Rider> rider,
         std::shared_ptr<Driver> driver,
         RideStatus status,
         double fare,
         Location pickup,
         Location dropoff,
         TimePoint requestedAt,
         TimePoint startedAt,
         TimePoint completedAt);

    const std::string& id() const;
    const std::shared_ptr<Rider>& rider() const;
    const std::shared_ptr<Driver>& driver() const;
    RideStatus status() const;
    double fare() const;
    const Location& pickup() const;
    const Location& dropoff() const;
    TimePoint requestedAt() const;
    TimePoint startedAt() const;
    TimePoint completedAt() const;

    void setId(std::string id);
    void setRider(std::shared_ptr<Rider> rider);
    void setDriver(std::shared_ptr<Driver> driver);
    void setStatus(RideStatus status);
    void setFare(double fare);
    void setPickup(Location pickup);
    void setDropoff(Location dropoff);
    void setRequestedAt(TimePoint t);
    void setStartedAt(TimePoint t);
    void setCompletedAt(TimePoint t);

private:
    std::string id_;
    std::shared_ptr<Rider> rider_;
    std::shared_ptr<Driver> driver_;
    RideStatus status_;
    double fare_;
    Location pickup_;
    Location dropoff_;
    TimePoint requestedAt_;
    TimePoint startedAt_;
    TimePoint completedAt_;
};

}  // namespace uber
