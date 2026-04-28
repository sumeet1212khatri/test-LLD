#pragma once
#include "core/types.hpp"
#include "core/order_book.hpp"
#include "risk/risk_manager.hpp"
#include "journal/journal.hpp"
#include "strategy/strategy.hpp"
#include <unordered_map>
#include <atomic>
#include <memory>
#include <mutex>
#include <deque>
#include <functional>

namespace hft {

struct BacktestResult {
    std::string strategy_name;
    int64_t     ticks_processed    = 0;
    int64_t     orders_submitted   = 0;
    int64_t     orders_filled      = 0;
    int64_t     orders_rejected    = 0;
    int64_t     trades_count       = 0;
    double      total_pnl          = 0;
    double      realized_pnl       = 0;
    double      unrealized_pnl     = 0;
    double      max_drawdown       = 0;
    double      sharpe             = 0;
    LatencyStats order_latency;
    LatencyStats tick_latency;
    double      throughput_eps     = 0; // events per second
    std::vector<double> equity_curve;
    std::vector<Trade>  trades;
};

class ExchangeSimulator {
public:
    explicit ExchangeSimulator(RiskLimits limits = {});

    // Add a symbol to trade
    void add_symbol(const std::string& symbol);

    // Submit order → goes through risk → matching engine
    // Returns {approved, reject_reason, trades}
    struct SubmitResult {
        bool   approved;
        std::string reject_reason;
        std::vector<Trade> trades;
    };
    SubmitResult submit_order(Order& order, double market_price = 0.0);
    bool cancel_order(const std::string& symbol, OrderId id);
    bool modify_order(const std::string& symbol, OrderId id, Price new_price, Quantity new_qty);

    // Feed a market data tick
    void on_tick(const MarketDataTick& tick);

    // Snapshot
    OrderBookSnapshot get_snapshot(const std::string& symbol, int depth = 10) const;
    std::vector<std::string> symbols() const;

    // State
    Position get_position(const std::string& symbol) const;
    std::unordered_map<std::string, Position> all_positions() const;
    double total_pnl() const;
    RiskManager& risk() { return risk_; }

    // Callbacks
    void set_trade_callback(TradeCallback cb)  { trade_cb_ = std::move(cb); }
    void set_order_callback(OrderCallback cb)  { order_cb_ = std::move(cb); }

    // Stats
    LatencyStats& order_latency() { return order_lat_; }
    LatencyStats& tick_latency()  { return tick_lat_; }
    int64_t total_orders()  const { return total_orders_; }
    int64_t total_trades()  const { return total_trades_; }
    int64_t total_rejects() const { return total_rejects_; }

    OrderId next_order_id() { return ++next_oid_; }

private:
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> books_;
    std::unordered_map<std::string, Order> order_store_; // owns all orders
    RiskManager risk_;
    BinaryJournal journal_;
    mutable std::mutex mtx_;

    TradeCallback trade_cb_;
    OrderCallback order_cb_;

    std::atomic<OrderId> next_oid_{1000};
    LatencyStats order_lat_, tick_lat_;
    int64_t total_orders_  = 0;
    int64_t total_trades_  = 0;
    int64_t total_rejects_ = 0;
};

// ─── Backtester ───────────────────────────────────────────────────────────────
class Backtester {
public:
    explicit Backtester(RiskLimits limits = {});

    void set_strategy(std::unique_ptr<IStrategy> strategy);
    void add_symbol(const std::string& symbol);

    BacktestResult run(const std::vector<MarketDataTick>& ticks,
                       bool enable_journal = true,
                       const std::string& journal_path = "replay.journal");

    // Replay a previously recorded journal
    BacktestResult replay_journal(const std::string& path);

private:
    std::unique_ptr<IStrategy>  strategy_;
    std::unique_ptr<ExchangeSimulator> sim_;
    RiskLimits limits_;
    std::vector<std::string> symbols_;

    void process_signals(const std::vector<StrategySignal>& sigs,
                         const MarketDataTick& tick,
                         BacktestResult& result,
                         std::unordered_map<OrderId, Order>& live_orders);
};

} // namespace hft
