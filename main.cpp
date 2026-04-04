#include <iostream>
#include <memory>
#include <string>

#include "concurrency/ThreadPool.h"
#include "core/services/DispatchSystem.h"
#include "core/services/DriverService.h"
#include "core/services/RiderService.h"
#include "engine/matching/MatchingEngine.h"
#include "engine/pricing/PerKmPricingStrategy.h"
#include "engine/pricing/PricingEngine.h"
#include "engine/pricing/SurgePricingStrategy.h"
#include "simulation/MetricsCollector.h"
#include "simulation/RideSimulator.h"

int main(int argc, char* argv[]) {
    const std::string metricsPath =
        (argc >= 2) ? std::string(argv[1]) : std::string("../frontend/simulation_metrics.js");
    auto riderService = std::make_shared<uber::RiderService>();
    auto driverService = std::make_shared<uber::DriverService>();
    auto matchingEngine = std::make_shared<uber::MatchingEngine>();

    auto baselineStrategy = std::make_shared<uber::PerKmPricingStrategy>(35.0, 11.5);
    auto surgeStrategy = std::make_shared<uber::SurgePricingStrategy>(35.0);
    auto pricingEngine = std::make_shared<uber::PricingEngine>(baselineStrategy);

    auto dispatch = std::make_shared<uber::DispatchSystem>(riderService,
                                                           driverService,
                                                           matchingEngine,
                                                           pricingEngine,
                                                           baselineStrategy,
                                                           surgeStrategy);
    dispatch->attachToRiderService();

    uber::ThreadPool pool(4);

    uber::RideSimulator simulator(dispatch, pricingEngine, surgeStrategy, &pool);
    simulator.simulate(500, 50);

    uber::MetricsCollector::instance().printReport();
    uber::MetricsCollector::instance().writeFrontendJavaScript(metricsPath);

    std::cout << "\nWrote dashboard data to " << metricsPath << '\n';

    return 0;
}
