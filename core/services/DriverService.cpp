#include "DriverService.h"

#include "../models/Driver.h"
#include "../models/Location.h"

namespace uber {

DriverService::DriverService() = default;

void DriverService::subscribe(std::shared_ptr<IObserver> observer) {
    if (!observer) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    observers_.push_back(std::move(observer));
}

void DriverService::registerDriver(std::shared_ptr<Driver> driver) {
    if (!driver) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    drivers_[driver->id()] = std::move(driver);
}

void DriverService::updateLocation(const std::string& driverId, const Location& location) {
    std::shared_ptr<Driver> driverCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = drivers_.find(driverId);
        if (it == drivers_.end() || !it->second) {
            return;
        }
        it->second->setLocation(location);
        driverCopy = it->second;
    }
    notifyObservers(*driverCopy);
}

void DriverService::toggleAvailability(const std::string& driverId) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = drivers_.find(driverId);
    if (it == drivers_.end() || !it->second) {
        return;
    }
    it->second->setAvailable(!it->second->isAvailable());
}

void DriverService::notifyObservers(const std::string& driverId) {
    std::shared_ptr<Driver> driverCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = drivers_.find(driverId);
        if (it == drivers_.end() || !it->second) {
            return;
        }
        driverCopy = it->second;
    }
    notifyObservers(*driverCopy);
}

std::shared_ptr<Driver> DriverService::findDriver(const std::string& driverId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = drivers_.find(driverId);
    if (it == drivers_.end()) {
        return nullptr;
    }
    return it->second;
}

int DriverService::countAvailableDrivers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int count = 0;
    for (const auto& entry : drivers_) {
        const auto& driver = entry.second;
        if (driver && driver->isAvailable()) {
            ++count;
        }
    }
    return count;
}

void DriverService::notifyObservers(Driver& driver) {
    std::vector<std::shared_ptr<IObserver>> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = observers_;
    }
    for (const auto& observer : snapshot) {
        if (observer) {
            observer->onDriverLocationUpdate(driver);
        }
    }
}

}  // namespace uber
