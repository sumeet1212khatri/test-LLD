#include "Driver.h"

#include "Ride.h"

namespace uber {

Driver::Driver()
    : id_(),
      name_(),
      rating_(0.0F),
      location_(),
      isAvailable_(true),
      currentRide_(nullptr) {}

Driver::Driver(std::string id,
               std::string name,
               float rating,
               Location location,
               bool isAvailable,
               std::shared_ptr<Ride> currentRide)
    : id_(std::move(id)),
      name_(std::move(name)),
      rating_(rating),
      location_(std::move(location)),
      isAvailable_(isAvailable),
      currentRide_(std::move(currentRide)) {}

const std::string& Driver::id() const {
    return id_;
}

const std::string& Driver::name() const {
    return name_;
}

float Driver::rating() const {
    return rating_;
}

const Location& Driver::location() const {
    return location_;
}

bool Driver::isAvailable() const {
    return isAvailable_;
}

const std::shared_ptr<Ride>& Driver::currentRide() const {
    return currentRide_;
}

void Driver::setId(std::string id) {
    id_ = std::move(id);
}

void Driver::setName(std::string name) {
    name_ = std::move(name);
}

void Driver::setRating(float rating) {
    rating_ = rating;
}

void Driver::setLocation(Location location) {
    location_ = std::move(location);
}

void Driver::setAvailable(bool available) {
    isAvailable_ = available;
}

void Driver::setCurrentRide(std::shared_ptr<Ride> ride) {
    currentRide_ = std::move(ride);
}

}  // namespace uber
