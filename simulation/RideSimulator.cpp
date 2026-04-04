#include "RideSimulator.h"

#include <atomic>
#include <chrono>
#include <random>
#include <thread>

#include "../core/models/Location.h"
#include "../core/models/Ride.h"
#include "../core/models/Rider.h"
#include "../core/services/DispatchSystem.h"
#include "../engine/pricing/SurgePricingStrategy.h"
#include "../engine/state/RideStateMachine.h"
#include "../concurrency/ThreadPool.h"
#include "MetricsCollector.h"

namespace uber {

RideSimulator::RideSimulator(std::shared_ptr<DispatchSystem> dispatch,
                             std::shared_ptr<PricingEngine> pricing,
                             std::shared_ptr<SurgePricingStrategy> surgeStrategy,
                             ThreadPool* threadPool)
    : dispatch_(std::move(dispatch)),
      pricing_(std::move(pricing)),
      surgeStrategy_(std::move(surgeStrategy)),
      pool_(threadPool),
      rng_(std::random_device{}()),
      latDist_(12.9, 13.1),
      lngDist_(77.5, 77.7),
      ratingDist_(4.0, 5.0),
      premiumDist_(0.3),
      tripMsDist_(50, 250) {}

RideSimulator::RandomLocation RideSimulator::randomLocation() {
    std::lock_guard<std::mutex> lock(rngMutex_);
    RandomLocation loc;
    loc.lat = latDist_(rng_);
    loc.lng = lngDist_(rng_);
    return loc;
}

float RideSimulator::randomRating() {
    std::lock_guard<std::mutex> lock(rngMutex_);
    return static_cast<float>(ratingDist_(rng_));
}

void RideSimulator::registerDrivers(int count) {
    registeredDrivers_.clear();
    registeredDrivers_.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const RandomLocation loc = randomLocation();
        const float rating = randomRating();
        auto driver = std::make_shared<Driver>(
            "sim-driver-" + std::to_string(i),
            "Driver " + std::to_string(i),
            rating,
            Location(loc.lat, loc.lng),
            true,
            nullptr);
        registeredDrivers_.push_back(driver);
        dispatch_->registerDriver(driver);
    }
}

void RideSimulator::registerRiders(int count) {
    for (int i = 0; i < count; ++i) {
        const RandomLocation loc = randomLocation();
        RiderTier tier = RiderTier::STANDARD;
        {
            std::lock_guard<std::mutex> lock(rngMutex_);
            tier = premiumDist_(rng_) ? RiderTier::PREMIUM : RiderTier::STANDARD;
        }
        auto rider = std::make_shared<Rider>("sim-rider-" + std::to_string(i),
                                             "Rider " + std::to_string(i),
                                             tier,
                                             Location(loc.lat, loc.lng));
        dispatch_->registerRider(rider);
    }
}

void RideSimulator::runOneRide(int rideIndex) {
    const std::string riderId = "sim-rider-" + std::to_string(rideIndex);
    const RandomLocation pickup = randomLocation();
    const RandomLocation dropoff = randomLocation();

    const auto tRequest = std::chrono::steady_clock::now();
    std::shared_ptr<Ride> ride;
    bool isSurge = false;

    {
        std::lock_guard<std::mutex> lock(coordinationMutex_);
        ride = dispatch_->requestRide(riderId,
                                      Location(pickup.lat, pickup.lng),
                                      Location(dropoff.lat, dropoff.lng));
        if (ride) {
            isSurge = (pricing_->strategy().get() == surgeStrategy_.get());
        }
    }

    const auto tAssigned = std::chrono::steady_clock::now();
    const double waitSec = std::chrono::duration<double>(tAssigned - tRequest).count();

    if (!ride) {
        MetricsCollector::instance().recordCancellation();
        return;
    }

    MetricsCollector::instance().recordRide(ride->id(), waitSec, ride->fare(), isSurge);

    int tripMs = 150;
    {
        std::lock_guard<std::mutex> lock(rngMutex_);
        tripMs = tripMsDist_(rng_);
    }

    {
        std::lock_guard<std::mutex> lock(coordinationMutex_);
        RideStateMachine::transition(ride, RideStatus::IN_PROGRESS);
        ride->setStartedAt(std::chrono::system_clock::now());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(tripMs));

    {
        std::lock_guard<std::mutex> lock(coordinationMutex_);
        ride->setCompletedAt(std::chrono::system_clock::now());
        RideStateMachine::transition(ride, RideStatus::COMPLETED);
        if (const auto driver = ride->driver()) {
            driver->setAvailable(true);
            driver->setCurrentRide(nullptr);
        }
    }

    {
        std::lock_guard<std::mutex> lock(busyMutex_);
        totalDriverBusySeconds_ += static_cast<double>(tripMs) / 1000.0;
    }

    {
        std::lock_guard<std::mutex> lock(completionMutex_);
        const double offset = std::chrono::duration<double>(std::chrono::steady_clock::now() - simStart_)
                                  .count();
        rideCompletionOffsetsSeconds_.push_back(offset);
    }
}

void RideSimulator::simulate(int numRides, int numDrivers) {
    MetricsCollector::instance().reset();
    MetricsCollector::instance().beginSimulation(numRides);

    rideCompletionOffsetsSeconds_.clear();
    totalDriverBusySeconds_ = 0.0;

    registerDrivers(numDrivers);
    registerRiders(numRides);

    simStart_ = std::chrono::steady_clock::now();

    std::atomic<int> finished{0};

    for (int i = 0; i < numRides; ++i) {
        pool_->enqueue([this, i, &finished]() {
            runOneRide(i);
            finished.fetch_add(1, std::memory_order_acq_rel);
        });
    }

    while (finished.load(std::memory_order_acquire) < numRides) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    const auto simEnd = std::chrono::steady_clock::now();
    const double wallSec = std::chrono::duration<double>(simEnd - simStart_).count();

    double busyTotal = 0.0;
    {
        std::lock_guard<std::mutex> lock(busyMutex_);
        busyTotal = totalDriverBusySeconds_;
    }

    double util = 0.0;
    if (wallSec > 0.0 && numDrivers > 0) {
        util = (busyTotal / (static_cast<double>(numDrivers) * wallSec)) * 100.0;
        if (util > 100.0) {
            util = 100.0;
        }
    }

    MetricsCollector::instance().setDriverUtilizationPercent(util);
    MetricsCollector::instance().setSimulationWallSeconds(wallSec);

    std::vector<int> buckets;
    {
        std::lock_guard<std::mutex> lock(completionMutex_);
        int maxMin = 0;
        for (double t : rideCompletionOffsetsSeconds_) {
            const int m = static_cast<int>(t / 60.0);
            if (m > maxMin) {
                maxMin = m;
            }
        }
        buckets.assign(static_cast<std::size_t>(maxMin) + 1, 0);
        for (double t : rideCompletionOffsetsSeconds_) {
            const int m = static_cast<int>(t / 60.0);
            if (m >= 0 && static_cast<std::size_t>(m) < buckets.size()) {
                buckets[static_cast<std::size_t>(m)]++;
            }
        }
    }
    MetricsCollector::instance().setRidesPerMinuteBuckets(std::move(buckets));

    std::vector<double> lats;
    std::vector<double> lngs;
    std::vector<bool> hot;
    {
        std::lock_guard<std::mutex> rngLock(rngMutex_);
        std::bernoulli_distribution surgeZone(0.25);
        for (const auto& driver : registeredDrivers_) {
            if (!driver) {
                continue;
            }
            lats.push_back(driver->location().latitude());
            lngs.push_back(driver->location().longitude());
            hot.push_back(surgeZone(rng_));
        }
    }
    MetricsCollector::instance().setDriverMapDots(std::move(lats), std::move(lngs), std::move(hot));
}

}  // namespace uber
