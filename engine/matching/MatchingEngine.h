#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../../core/interfaces/IMatchingEngine.h"

namespace uber {

class Driver;

class MatchingEngine : public IMatchingEngine {
public:
    MatchingEngine();
    ~MatchingEngine() override = default;

    void registerDriver(std::shared_ptr<Driver> driver);

    std::shared_ptr<Ride> requestRide(const std::shared_ptr<Rider>& rider,
                                      const Location& pickup,
                                      const Location& dropoff) override;

private:
    std::vector<std::shared_ptr<Driver>> snapshotAvailableDriversLocked();

    mutable std::mutex driversMutex_;
    std::vector<std::shared_ptr<Driver>> drivers_;
};

}  // namespace uber
