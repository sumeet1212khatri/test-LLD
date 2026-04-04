#include "MetricsCollector.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace uber {

MetricsCollector& MetricsCollector::instance() {
    static MetricsCollector collector;
    return collector;
}

void MetricsCollector::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    totalRidesPlanned_ = 0;
    completed_ = 0;
    cancelled_ = 0;
    totalWaitSeconds_ = 0.0;
    totalFare_ = 0.0;
    surgeRideCount_ = 0;
    driverUtilizationPercent_ = 0.0;
    simulationWallSeconds_ = 0.0;
    ridesPerMinute_.clear();
    driverLats_.clear();
    driverLngs_.clear();
    driverSurgeHot_.clear();
}

void MetricsCollector::beginSimulation(int totalRidesPlanned) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalRidesPlanned_ = totalRidesPlanned;
}

void MetricsCollector::recordRide(const std::string& rideId,
                                  double waitTimeSeconds,
                                  double fareAmount,
                                  bool isSurge) {
    static_cast<void>(rideId);
    std::lock_guard<std::mutex> lock(mutex_);
    ++completed_;
    totalWaitSeconds_ += waitTimeSeconds;
    totalFare_ += fareAmount;
    if (isSurge) {
        ++surgeRideCount_;
    }
}

void MetricsCollector::recordCancellation() {
    std::lock_guard<std::mutex> lock(mutex_);
    ++cancelled_;
}

void MetricsCollector::setDriverUtilizationPercent(double percent) {
    std::lock_guard<std::mutex> lock(mutex_);
    driverUtilizationPercent_ = percent;
}

void MetricsCollector::setSimulationWallSeconds(double seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    simulationWallSeconds_ = seconds;
}

void MetricsCollector::setRidesPerMinuteBuckets(std::vector<int> buckets) {
    std::lock_guard<std::mutex> lock(mutex_);
    ridesPerMinute_ = std::move(buckets);
}

void MetricsCollector::setDriverMapDots(std::vector<double> lats,
                                        std::vector<double> lngs,
                                        std::vector<bool> surgeHot) {
    std::lock_guard<std::mutex> lock(mutex_);
    driverLats_ = std::move(lats);
    driverLngs_ = std::move(lngs);
    driverSurgeHot_ = std::move(surgeHot);
}

void MetricsCollector::printReport() const {
    std::lock_guard<std::mutex> lock(mutex_);

    const double avgWaitMinutes =
        completed_ > 0 ? (totalWaitSeconds_ / static_cast<double>(completed_)) / 60.0 : 0.0;
    const double avgFare = completed_ > 0 ? totalFare_ / static_cast<double>(completed_) : 0.0;

    std::cout << '\n';
    std::cout << "Simulating " << totalRidesPlanned_ << " rides...\n";
    std::cout << "✅ Completed: " << completed_ << " | ❌ Cancelled: " << cancelled_ << '\n';
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "⏱  Avg wait time: " << avgWaitMinutes << " min\n";
    std::cout << std::setprecision(0);
    std::cout << "🚗 Driver utilization: " << driverUtilizationPercent_ << "%\n";
    std::cout << "💥 Surge activated: " << surgeRideCount_ << " times\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "💰 Avg fare: ₹" << avgFare << '\n';
    std::cout << std::flush;
}

void MetricsCollector::writeFrontendJavaScript(const std::string& filePath) const {
    std::vector<int> rpmCopy;
    double avgWaitMin = 0.0;
    double avgFare = 0.0;
    std::int64_t completed = 0;
    std::int64_t cancelled = 0;
    std::int64_t surgeCount = 0;
    int totalPlanned = 0;
    double util = 0.0;
    double wallSec = 0.0;
    std::vector<double> lats;
    std::vector<double> lngs;
    std::vector<bool> hot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        rpmCopy = ridesPerMinute_;
        completed = completed_;
        cancelled = cancelled_;
        surgeCount = surgeRideCount_;
        totalPlanned = totalRidesPlanned_;
        util = driverUtilizationPercent_;
        wallSec = simulationWallSeconds_;
        lats = driverLats_;
        lngs = driverLngs_;
        hot = driverSurgeHot_;
        if (completed_ > 0) {
            avgWaitMin = (totalWaitSeconds_ / static_cast<double>(completed_)) / 60.0;
            avgFare = totalFare_ / static_cast<double>(completed_);
        }
    }

    std::ostringstream js;
    js << "window.UBER_SIM_METRICS = {\n";
    js << "  totalRides: " << totalPlanned << ",\n";
    js << "  completed: " << completed << ",\n";
    js << "  cancelled: " << cancelled << ",\n";
    js << "  avgWaitMinutes: " << std::fixed << std::setprecision(4) << avgWaitMin << ",\n";
    js << "  driverUtilization: " << std::setprecision(2) << util << ",\n";
    js << "  surgeEvents: " << surgeCount << ",\n";
    js << "  avgFare: " << std::setprecision(2) << avgFare << ",\n";
    js << "  simulationWallSeconds: " << std::setprecision(4) << wallSec << ",\n";
    js << "  ridesPerMinute: [";
    for (std::size_t i = 0; i < rpmCopy.size(); ++i) {
        if (i > 0) {
            js << ", ";
        }
        js << rpmCopy[i];
    }
    js << "],\n";
    js << "  driverDots: [\n";
    for (std::size_t i = 0; i < lats.size() && i < lngs.size(); ++i) {
        const bool s = i < hot.size() ? hot[i] : false;
        js << "    { lat: " << std::setprecision(6) << lats[i] << ", lng: " << lngs[i]
           << ", surge: " << (s ? "true" : "false") << " }";
        if (i + 1 < lats.size()) {
            js << ",";
        }
        js << "\n";
    }
    js << "  ]\n";
    js << "};\n";

    std::ofstream out(filePath.c_str(), std::ios::binary | std::ios::trunc);
    if (!out) {
        std::cerr << "Warning: could not write " << filePath << '\n';
        return;
    }
    out << js.str();
}

}  // namespace uber
