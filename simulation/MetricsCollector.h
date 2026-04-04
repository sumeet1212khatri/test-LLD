#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace uber {

class MetricsCollector {
public:
    static MetricsCollector& instance();

    MetricsCollector(const MetricsCollector&) = delete;
    MetricsCollector& operator=(const MetricsCollector&) = delete;

    void reset();
    void beginSimulation(int totalRidesPlanned);

    void recordRide(const std::string& rideId,
                    double waitTimeSeconds,
                    double fareAmount,
                    bool isSurge);

    void recordCancellation();

    void setDriverUtilizationPercent(double percent);

    void setSimulationWallSeconds(double seconds);

    void setRidesPerMinuteBuckets(std::vector<int> buckets);

    void setDriverMapDots(std::vector<double> lats,
                          std::vector<double> lngs,
                          std::vector<bool> surgeHot);

    void printReport() const;

    void writeFrontendJavaScript(const std::string& filePath) const;

private:
    MetricsCollector() = default;

    mutable std::mutex mutex_;

    int totalRidesPlanned_{0};
    std::int64_t completed_{0};
    std::int64_t cancelled_{0};
    double totalWaitSeconds_{0.0};
    double totalFare_{0.0};
    std::int64_t surgeRideCount_{0};
    double driverUtilizationPercent_{0.0};
    double simulationWallSeconds_{0.0};
    std::vector<int> ridesPerMinute_;
    std::vector<double> driverLats_;
    std::vector<double> driverLngs_;
    std::vector<bool> driverSurgeHot_;
};

}  // namespace uber
