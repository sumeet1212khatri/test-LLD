#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace uber {

class Rider;
class Ride;
class Location;
class DispatchSystem;

class RiderService {
public:
    RiderService();

    void setDispatch(std::weak_ptr<DispatchSystem> dispatch);

    void registerRider(std::shared_ptr<Rider> rider);
    std::shared_ptr<Ride> requestRide(const std::string& riderId,
                                      const Location& pickup,
                                      const Location& dropoff);
    std::vector<std::shared_ptr<Ride>> getRideHistory(const std::string& riderId) const;

    std::shared_ptr<Rider> findRider(const std::string& riderId) const;

private:
    std::weak_ptr<DispatchSystem> dispatch_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Rider>> riders_;
};

}  // namespace uber
