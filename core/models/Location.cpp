#include "Location.h"

#include <algorithm>
#include <cmath>

namespace uber {

Location::Location() : latitude_(0.0), longitude_(0.0) {}

Location::Location(double latitude, double longitude)
    : latitude_(latitude), longitude_(longitude) {}

double Location::latitude() const {
    return latitude_;
}

double Location::longitude() const {
    return longitude_;
}

void Location::setLatitude(double latitude) {
    latitude_ = latitude;
}

void Location::setLongitude(double longitude) {
    longitude_ = longitude;
}

double Location::deg2rad(double degrees) {
    return degrees * (3.14159265358979323846 / 180.0);
}

double Location::distanceTo(const Location& other) const {
    constexpr double earthRadiusKm = 6371.0;
    const double lat1 = deg2rad(latitude_);
    const double lat2 = deg2rad(other.latitude_);
    const double dLat = deg2rad(other.latitude_ - latitude_);
    const double dLon = deg2rad(other.longitude_ - longitude_);

    const double sinHalfDLat = std::sin(dLat / 2.0);
    const double sinHalfDLon = std::sin(dLon / 2.0);
    const double a = sinHalfDLat * sinHalfDLat +
                     std::cos(lat1) * std::cos(lat2) * sinHalfDLon * sinHalfDLon;
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(std::max(0.0, 1.0 - a)));
    return earthRadiusKm * c;
}

}  // namespace uber
