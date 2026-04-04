#include "Ride.h"

#include "Driver.h"
#include "Rider.h"

namespace uber {

Ride::Ride()
    : id_(),
      rider_(nullptr),
      driver_(nullptr),
      status_(RideStatus::REQUESTED),
      fare_(0.0),
      pickup_(),
      dropoff_(),
      requestedAt_(Clock::now()),
      startedAt_(),
      completedAt_() {}

Ride::Ride(std::string id,
           std::shared_ptr<Rider> rider,
           std::shared_ptr<Driver> driver,
           RideStatus status,
           double fare,
           Location pickup,
           Location dropoff,
           TimePoint requestedAt,
           TimePoint startedAt,
           TimePoint completedAt)
    : id_(std::move(id)),
      rider_(std::move(rider)),
      driver_(std::move(driver)),
      status_(status),
      fare_(fare),
      pickup_(std::move(pickup)),
      dropoff_(std::move(dropoff)),
      requestedAt_(requestedAt),
      startedAt_(startedAt),
      completedAt_(completedAt) {}

const std::string& Ride::id() const {
    return id_;
}

const std::shared_ptr<Rider>& Ride::rider() const {
    return rider_;
}

const std::shared_ptr<Driver>& Ride::driver() const {
    return driver_;
}

RideStatus Ride::status() const {
    return status_;
}

double Ride::fare() const {
    return fare_;
}

const Location& Ride::pickup() const {
    return pickup_;
}

const Location& Ride::dropoff() const {
    return dropoff_;
}

Ride::TimePoint Ride::requestedAt() const {
    return requestedAt_;
}

Ride::TimePoint Ride::startedAt() const {
    return startedAt_;
}

Ride::TimePoint Ride::completedAt() const {
    return completedAt_;
}

void Ride::setId(std::string id) {
    id_ = std::move(id);
}

void Ride::setRider(std::shared_ptr<Rider> rider) {
    rider_ = std::move(rider);
}

void Ride::setDriver(std::shared_ptr<Driver> driver) {
    driver_ = std::move(driver);
}

void Ride::setStatus(RideStatus status) {
    status_ = status;
}

void Ride::setFare(double fare) {
    fare_ = fare;
}

void Ride::setPickup(Location pickup) {
    pickup_ = std::move(pickup);
}

void Ride::setDropoff(Location dropoff) {
    dropoff_ = std::move(dropoff);
}

void Ride::setRequestedAt(TimePoint t) {
    requestedAt_ = t;
}

void Ride::setStartedAt(TimePoint t) {
    startedAt_ = t;
}

void Ride::setCompletedAt(TimePoint t) {
    completedAt_ = t;
}

}  // namespace uber
