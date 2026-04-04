#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "../interfaces/IObserver.h"

namespace uber {

class Driver;
class Location;

class DriverService {
public:
    DriverService();

    void subscribe(std::shared_ptr<IObserver> observer);

    void registerDriver(std::shared_ptr<Driver> driver);
    void updateLocation(const std::string& driverId, const Location& location);
    void toggleAvailability(const std::string& driverId);

    void notifyObservers(const std::string& driverId);

    std::shared_ptr<Driver> findDriver(const std::string& driverId) const;
    int countAvailableDrivers() const;

private:
    void notifyObservers(Driver& driver);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Driver>> drivers_;
    std::vector<std::shared_ptr<IObserver>> observers_;
};

}  // namespace uber
