#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

#include "../core/models/Driver.h"

namespace uber {

class DispatchSystem;
class PricingEngine;
class SurgePricingStrategy;
class ThreadPool;

class RideSimulator {
public:
    RideSimulator(std::shared_ptr<DispatchSystem> dispatch,
                  std::shared_ptr<PricingEngine> pricing,
                  std::shared_ptr<SurgePricingStrategy> surgeStrategy,
                  ThreadPool* threadPool);

    void simulate(int numRides, int numDrivers);

private:
    struct RandomLocation {
        double lat;
        double lng;
    };

    RandomLocation randomLocation();
    float randomRating();
    void registerDrivers(int count);
    void registerRiders(int count);

    void runOneRide(int rideIndex);

    std::shared_ptr<DispatchSystem> dispatch_;
    std::shared_ptr<PricingEngine> pricing_;
    std::shared_ptr<SurgePricingStrategy> surgeStrategy_;
    ThreadPool* pool_;

    std::mutex rngMutex_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> latDist_;
    std::uniform_real_distribution<double> lngDist_;
    std::uniform_real_distribution<double> ratingDist_;
    std::bernoulli_distribution premiumDist_;
    std::uniform_int_distribution<int> tripMsDist_;

    std::chrono::steady_clock::time_point simStart_{};

    std::mutex coordinationMutex_;
    std::vector<std::shared_ptr<Driver>> registeredDrivers_;

    std::mutex completionMutex_;
    std::vector<double> rideCompletionOffsetsSeconds_;
    std::mutex busyMutex_;
    double totalDriverBusySeconds_{0.0};
};

}  // namespace uber
