#include "PricingEngine.h"

namespace uber {

PricingEngine::PricingEngine() : strategy_(nullptr) {}

PricingEngine::PricingEngine(std::shared_ptr<IPricingStrategy> strategy)
    : strategy_(std::move(strategy)) {}

void PricingEngine::setStrategy(std::shared_ptr<IPricingStrategy> strategy) {
    strategy_ = std::move(strategy);
}

const std::shared_ptr<IPricingStrategy>& PricingEngine::strategy() const {
    return strategy_;
}

double PricingEngine::calculateFare(double distanceKm, int demandLevel) const {
    if (!strategy_) {
        return 0.0;
    }
    return strategy_->calculateFare(distanceKm, demandLevel);
}

}  // namespace uber
