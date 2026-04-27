#include "risk/risk_manager.hpp"
#include <cmath>
#include <algorithm>

namespace hft {

RiskManager::RiskManager(RiskLimits limits) : limits_(std::move(limits)) {}

int RiskManager::orders_in_last_second() {
    int64_t now = now_ns();
    int64_t cutoff = now - 1'000'000'000LL; // 1 second in ns
    while (!order_timestamps_ns_.empty() && order_timestamps_ns_.front() < cutoff)
        order_timestamps_ns_.pop_front();
    return (int)order_timestamps_ns_.size();
}

RiskResult RiskManager::check_order(const Order& order, double current_price) {
    std::lock_guard<std::mutex> lock(mtx_);
    ++stats_checked_;

    if (!limits_.enabled) return {true, RiskRejectReason::NONE, "OK"};

    RiskResult r;

    // Basic validation
    if (order.qty <= 0) {
        ++stats_rejected_;
        return {false, RiskRejectReason::INVALID_QTY, "Quantity must be positive"};
    }
    if (order.type == OrderType::LIMIT && order.price <= 0) {
        ++stats_rejected_;
        return {false, RiskRejectReason::INVALID_PRICE, "Limit price must be positive"};
    }

    // Max order size
    if (order.qty > limits_.max_order_qty) {
        ++stats_rejected_;
        return {false, RiskRejectReason::MAX_ORDER_QTY,
            "Order qty " + std::to_string(order.qty) + " > limit " + std::to_string(limits_.max_order_qty)};
    }

    // Notional check
    double price_usd = (current_price > 0) ? current_price : from_price(order.price);
    double notional  = price_usd * order.qty;
    if (notional > limits_.max_notional_usd) {
        ++stats_rejected_;
        return {false, RiskRejectReason::MAX_NOTIONAL,
            "Notional $" + std::to_string(notional) + " > limit $" + std::to_string(limits_.max_notional_usd)};
    }

    // Position limit
    auto pit = positions_.find(order.symbol);
    Quantity current_pos = (pit != positions_.end()) ? pit->second.net_qty : 0;
    Quantity projected   = current_pos + (order.side == Side::BUY ? order.qty : -order.qty);
    if (std::abs(projected) > limits_.max_position) {
        ++stats_rejected_;
        return {false, RiskRejectReason::MAX_POSITION,
            "Projected position " + std::to_string(projected) + " > limit " + std::to_string(limits_.max_position)};
    }

    // Order rate
    int rate = orders_in_last_second();
    if (rate >= limits_.max_orders_per_sec) {
        ++stats_rejected_;
        return {false, RiskRejectReason::MAX_ORDER_RATE,
            "Order rate " + std::to_string(rate) + "/s > limit " + std::to_string(limits_.max_orders_per_sec)};
    }

    order_timestamps_ns_.push_back(now_ns());
    r.approved = true;
    r.message  = "OK";
    return r;
}

void RiskManager::on_fill(const std::string& symbol, Side side, Quantity qty, double fill_price) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto& pos = positions_[symbol];
    pos.symbol = symbol;

    double signed_qty = (side == Side::BUY) ? qty : -qty;

    // Update avg price (FIFO-ish: when adding to position)
    if ((side == Side::BUY && pos.net_qty >= 0) ||
        (side == Side::SELL && pos.net_qty <= 0)) {
        // Adding to position
        double total_cost = pos.avg_price * std::abs(pos.net_qty) + fill_price * qty;
        pos.net_qty += (int64_t)signed_qty;
        if (pos.net_qty != 0)
            pos.avg_price = total_cost / std::abs(pos.net_qty);
    } else {
        // Reducing/flipping
        double realized = (fill_price - pos.avg_price) * qty *
                          (pos.net_qty > 0 ? 1.0 : -1.0);
        pos.realized_pnl += realized;
        pos.net_qty += (int64_t)signed_qty;
        if (pos.net_qty == 0) pos.avg_price = 0;
    }

    // Update unrealized
    if (pos.net_qty != 0) {
        double mkt = market_prices_.count(symbol) ? market_prices_[symbol] : fill_price;
        pos.unrealized_pnl = (mkt - pos.avg_price) * pos.net_qty;
    } else {
        pos.unrealized_pnl = 0;
    }
}

void RiskManager::update_market_price(const std::string& sym, double px) {
    std::lock_guard<std::mutex> lock(mtx_);
    market_prices_[sym] = px;
    if (positions_.count(sym)) {
        auto& pos = positions_[sym];
        if (pos.net_qty != 0)
            pos.unrealized_pnl = (px - pos.avg_price) * pos.net_qty;
    }
}

void RiskManager::on_cancel(const std::string& symbol, const Order& order) {
    // Nothing to do for simple position risk, but could track reserved qty
    (void)symbol; (void)order;
}

Position RiskManager::get_position(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = positions_.find(symbol);
    if (it == positions_.end()) return Position{symbol, 0, 0, 0, 0};
    return it->second;
}

std::unordered_map<std::string, Position> RiskManager::all_positions() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return positions_;
}

double RiskManager::total_pnl() const {
    std::lock_guard<std::mutex> lock(mtx_);
    double total = 0;
    for (auto& [sym, pos] : positions_)
        total += pos.realized_pnl + pos.unrealized_pnl;
    return total;
}

} // namespace hft