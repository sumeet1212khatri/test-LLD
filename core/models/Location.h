#pragma once

namespace uber {

class Location {
public:
    Location();
    Location(double latitude, double longitude);

    double latitude() const;
    double longitude() const;

    void setLatitude(double latitude);
    void setLongitude(double longitude);

    double distanceTo(const Location& other) const;

private:
    static double deg2rad(double degrees);

    double latitude_;
    double longitude_;
};

}  // namespace uber
