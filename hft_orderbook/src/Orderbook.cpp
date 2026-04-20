#include "Orderbook.h"
#include <numeric>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────
Orderbook::Orderbook()
    : pruneThread_([this] { PruneGoodForDayOrders(); })
{}

Orderbook::~Orderbook() {
    shutdown_.store(true, std::memory_order_release);
    shutdownCV_.notify_one();
    pruneThread_.join();
}

// ─────────────────────────────────────────────────────────────────────────────
// GFD pruning thread: runs at market close (16:00 local)
// ─────────────────────────────────────────────────────────────────────────────
void Orderbook::PruneGoodForDayOrders() {
    using namespace std::chrono;
    while (true) {
        const auto now    = system_clock::now();
        const auto now_t  = system_clock::to_time_t(now);
        std::tm parts{};
#ifdef _WIN32
        localtime_s(&parts, &now_t);
#else
        localtime_r(&now_t, &parts);
#endif
        if (parts.tm_hour >= 16) parts.tm_mday += 1;
        parts.tm_hour = 16; parts.tm_min = 0; parts.tm_sec = 0;
        auto next = system_clock::from_time_t(mktime(&parts));
        auto till = next - now + milliseconds(100);

        {
            std::unique_lock lk{ordersMutex_};
            if (shutdown_.load(std::memory_order_acquire) ||
                shutdownCV_.wait_for(lk, till) == std::cv_status::no_timeout)
                return;
        }

        OrderIds ids;
        {
            std::scoped_lock lk{ordersMutex_};
            for (const auto& [id, entry] : orders_)
                if (entry.order->GetOrderType() == OrderType::GoodForDay)
                    ids.push_back(id);
        }
        CancelOrdersInternal(ids);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AddOrder - main entry point
// ─────────────────────────────────────────────────────────────────────────────
Trades Orderbook::AddOrder(OrderPointer order) {
    std::scoped_lock lk{ordersMutex_};
    stats_.totalOrders.fetch_add(1, std::memory_order_relaxed);
    stats_.seqNum.fetch_add(1, std::memory_order_relaxed);

    if (orders_.contains(order->GetOrderId()))
        return {};

    // ── PostOnly: reject if it would cross the spread ─────────────────────────
    if (order->GetOrderType() == OrderType::PostOnly) {
        if (CanMatch(order->GetSide(), order->GetPrice())) {
            order->SetStatus(OrderStatus::Rejected);
            return {};
        }
    }

    // ── StopLimit: park in stop order book if stop not triggered yet ──────────
    if (order->GetOrderType() == OrderType::StopLimit) {
        Price lastTrade = stats_.lastTradePrice.load(std::memory_order_relaxed);
        bool triggered = false;
        if (order->GetSide() == Side::Buy  && lastTrade >= order->GetStopPrice()) triggered = true;
        if (order->GetSide() == Side::Sell && lastTrade <= order->GetStopPrice()) triggered = true;
        if (!triggered) {
            if (order->GetSide() == Side::Buy)
                stopBuys_.emplace(order->GetStopPrice(), order);
            else
                stopSells_.emplace(order->GetStopPrice(), order);
            return {};  // parked - will inject when stop triggers
        }
        // Already triggered: convert to GTC limit and fall through
        order = std::make_shared<Order>(
            OrderType::GoodTillCancel,
            order->GetOrderId(), order->GetSide(), order->GetPrice(),
            order->GetRemainingQuantity());
    }

    // ── Market order: use worst available price to ensure full sweep ──────────
    if (order->GetOrderType() == OrderType::Market) {
        if (order->GetSide() == Side::Buy && !asks_.empty()) {
            const auto& [worstAsk, _] = *asks_.rbegin();
            order->ToGoodTillCancel(worstAsk);
        } else if (order->GetSide() == Side::Sell && !bids_.empty()) {
            const auto& [worstBid, _] = *bids_.rbegin();
            order->ToGoodTillCancel(worstBid);
        } else {
            return {};  // no liquidity
        }
    }

    // ── FAK / IOC: reject immediately if no match ─────────────────────────────
    if (order->GetOrderType() == OrderType::FillAndKill ||
        order->GetOrderType() == OrderType::ImmediateOrCancel) {
        if (!CanMatch(order->GetSide(), order->GetPrice()))
            return {};
    }

    // ── FOK: reject if cannot fully fill ─────────────────────────────────────
    if (order->GetOrderType() == OrderType::FillOrKill) {
        if (!CanFullyFill(order->GetSide(), order->GetPrice(), order->GetInitialQuantity()))
            return {};
    }

    // ── Insert into price level ───────────────────────────────────────────────
    OrderPointers::iterator it;
    if (order->GetSide() == Side::Buy) {
        auto& level = bids_[order->GetPrice()];
        level.push_back(order);
        it = std::prev(level.end());
    } else {
        auto& level = asks_[order->GetPrice()];
        level.push_back(order);
        it = std::prev(level.end());
    }
    orders_[order->GetOrderId()] = {order, it};
    OnOrderAdded(order);

    if (onAdded_) onAdded_(*order);

    return MatchOrders();
}

// ─────────────────────────────────────────────────────────────────────────────
// CancelOrder
// ─────────────────────────────────────────────────────────────────────────────
void Orderbook::CancelOrder(OrderId orderId) {
    std::scoped_lock lk{ordersMutex_};
    CancelOrderInternal(orderId);
}

void Orderbook::CancelOrdersInternal(const OrderIds& ids) {
    std::scoped_lock lk{ordersMutex_};
    for (auto id : ids) CancelOrderInternal(id);
}

void Orderbook::CancelOrderInternal(OrderId orderId) {
    if (!orders_.contains(orderId)) return;

    const auto [order, it] = orders_.at(orderId);
    orders_.erase(orderId);

    if (order->GetSide() == Side::Sell) {
        auto price = order->GetPrice();
        auto& level = asks_.at(price);
        level.erase(it);
        if (level.empty()) asks_.erase(price);
    } else {
        auto price = order->GetPrice();
        auto& level = bids_.at(price);
        level.erase(it);
        if (level.empty()) bids_.erase(price);
    }
    order->Cancel();
    OnOrderCancelled(order);
    if (onCancelled_) onCancelled_(*order);
    stats_.totalCancels.fetch_add(1, std::memory_order_relaxed);
}

// ─────────────────────────────────────────────────────────────────────────────
// ModifyOrder - cancel + re-add (price change loses time priority)
// ─────────────────────────────────────────────────────────────────────────────
Trades Orderbook::ModifyOrder(OrderModify mod) {
    OrderType type;
    {
        std::scoped_lock lk{ordersMutex_};
        if (!orders_.contains(mod.GetOrderId())) return {};
        type = orders_.at(mod.GetOrderId()).order->GetOrderType();
    }
    CancelOrder(mod.GetOrderId());
    return AddOrder(mod.ToOrderPointer(type));
}

// ─────────────────────────────────────────────────────────────────────────────
// Matching Engine - price-time priority
// ─────────────────────────────────────────────────────────────────────────────
Trades Orderbook::MatchOrders() {
    Trades trades;
    trades.reserve(32);

    while (!bids_.empty() && !asks_.empty()) {
        auto& [bidPrice, bidLevel] = *bids_.begin();
        auto& [askPrice, askLevel] = *asks_.begin();

        if (bidPrice < askPrice) break;  // no cross

        while (!bidLevel.empty() && !askLevel.empty()) {
            auto bid = bidLevel.front();
            auto ask = askLevel.front();

            Quantity qty = std::min(bid->GetRemainingQuantity(),
                                    ask->GetRemainingQuantity());

            bid->Fill(qty);
            ask->Fill(qty);

            // Remove fully filled orders
            if (bid->IsFilled()) {
                bidLevel.pop_front();
                orders_.erase(bid->GetOrderId());
            }
            if (ask->IsFilled()) {
                askLevel.pop_front();
                orders_.erase(ask->GetOrderId());
            }

            // Record trade at ask price (taker crosses to maker)
            auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
            Trade trade{
                TradeInfo{bid->GetOrderId(), bid->GetPrice(), qty},
                TradeInfo{ask->GetOrderId(), ask->GetPrice(), qty},
                ts
            };
            trades.push_back(trade);

            OnOrderMatched(bid->GetPrice(), qty, bid->IsFilled());
            OnOrderMatched(ask->GetPrice(), qty, ask->IsFilled());

            stats_.totalTrades.fetch_add(1, std::memory_order_relaxed);
            stats_.totalVolume.fetch_add(qty,  std::memory_order_relaxed);
            stats_.lastTradePrice.store(ask->GetPrice(), std::memory_order_relaxed);
            stats_.lastTradeQty.store(qty, std::memory_order_relaxed);

            if (onTrade_) onTrade_(trade);

            // Trigger any stop orders
            CheckAndTriggerStops(ask->GetPrice());
        }

        // Clean empty levels
        if (bidLevel.empty()) { bids_.erase(bidPrice); levelData_.erase(bidPrice); }
        if (askLevel.empty()) { asks_.erase(askPrice); levelData_.erase(askPrice); }
    }

    // Cancel remaining FAK / IOC orders
    auto cancelFAK = [&](auto& side) {
        if (!side.empty()) {
            auto& [_, level] = *side.begin();
            if (!level.empty()) {
                auto& front = level.front();
                if (front->GetOrderType() == OrderType::FillAndKill ||
                    front->GetOrderType() == OrderType::ImmediateOrCancel) {
                    CancelOrderInternal(front->GetOrderId());
                }
            }
        }
    };
    cancelFAK(bids_);
    cancelFAK(asks_);

    return trades;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stop order triggering
// ─────────────────────────────────────────────────────────────────────────────
void Orderbook::CheckAndTriggerStops(Price lastTrade) {
    std::vector<OrderPointer> toInject;

    // Buy stops: trigger when lastTrade >= stopPrice
    for (auto it = stopBuys_.begin(); it != stopBuys_.end(); ) {
        if (lastTrade >= it->first) {
            toInject.push_back(it->second);
            it = stopBuys_.erase(it);
        } else ++it;
    }
    // Sell stops: trigger when lastTrade <= stopPrice
    for (auto it = stopSells_.begin(); it != stopSells_.end(); ) {
        if (lastTrade <= it->first) {
            toInject.push_back(it->second);
            it = stopSells_.erase(it);
        } else ++it;
    }

    // Convert triggered stops to limit orders and insert
    for (auto& o : toInject) {
        auto limitOrder = std::make_shared<Order>(
            OrderType::GoodTillCancel,
            o->GetOrderId(), o->GetSide(), o->GetPrice(),
            o->GetRemainingQuantity());
        // Re-insert (mutex already held; call internal path)
        OrderPointers::iterator it;
        if (limitOrder->GetSide() == Side::Buy) {
            auto& level = bids_[limitOrder->GetPrice()];
            level.push_back(limitOrder);
            it = std::prev(level.end());
        } else {
            auto& level = asks_[limitOrder->GetPrice()];
            level.push_back(limitOrder);
            it = std::prev(level.end());
        }
        orders_[limitOrder->GetOrderId()] = {limitOrder, it};
        OnOrderAdded(limitOrder);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CanMatch / CanFullyFill
// ─────────────────────────────────────────────────────────────────────────────
bool Orderbook::CanMatch(Side side, Price price) const {
    if (side == Side::Buy) {
        if (asks_.empty()) return false;
        return price >= asks_.begin()->first;
    } else {
        if (bids_.empty()) return false;
        return price <= bids_.begin()->first;
    }
}

bool Orderbook::CanFullyFill(Side side, Price price, Quantity qty) const {
    if (!CanMatch(side, price)) return false;

    Quantity avail = 0;
    if (side == Side::Buy) {
        for (const auto& [lvlPrice, level] : asks_) {
            if (lvlPrice > price) break;
            for (const auto& o : level) avail += o->GetRemainingQuantity();
            if (avail >= qty) return true;
        }
    } else {
        for (const auto& [lvlPrice, level] : bids_) {
            if (lvlPrice < price) break;
            for (const auto& o : level) avail += o->GetRemainingQuantity();
            if (avail >= qty) return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Level data maintenance
// ─────────────────────────────────────────────────────────────────────────────
void Orderbook::UpdateLevelData(Price price, Quantity qty, LevelData::Action action) {
    auto& d = levelData_[price];
    switch (action) {
        case LevelData::Action::Add:
            d.quantity += qty; ++d.count; break;
        case LevelData::Action::Remove:
            d.quantity -= qty; --d.count; break;
        case LevelData::Action::Match:
            d.quantity -= qty; break;
    }
    if (d.count == 0) levelData_.erase(price);
}

void Orderbook::OnOrderAdded(OrderPointer order) {
    UpdateLevelData(order->GetPrice(), order->GetInitialQuantity(), LevelData::Action::Add);
}
void Orderbook::OnOrderCancelled(OrderPointer order) {
    UpdateLevelData(order->GetPrice(), order->GetRemainingQuantity(), LevelData::Action::Remove);
}
void Orderbook::OnOrderMatched(Price price, Quantity qty, bool fullyFilled) {
    UpdateLevelData(price, qty,
        fullyFilled ? LevelData::Action::Remove : LevelData::Action::Match);
}

// ─────────────────────────────────────────────────────────────────────────────
// Query methods
// ─────────────────────────────────────────────────────────────────────────────
std::size_t Orderbook::Size() const {
    std::scoped_lock lk{ordersMutex_};
    return orders_.size();
}

bool Orderbook::HasOrder(OrderId id) const {
    std::scoped_lock lk{ordersMutex_};
    return orders_.contains(id);
}

Price Orderbook::BestBid() const {
    std::scoped_lock lk{ordersMutex_};
    if (bids_.empty()) return Constants::InvalidPrice;
    return bids_.begin()->first;
}

Price Orderbook::BestAsk() const {
    std::scoped_lock lk{ordersMutex_};
    if (asks_.empty()) return Constants::InvalidPrice;
    return asks_.begin()->first;
}

Price Orderbook::MidPrice() const {
    auto bid = BestBid(), ask = BestAsk();
    if (bid == Constants::InvalidPrice || ask == Constants::InvalidPrice)
        return Constants::InvalidPrice;
    return (bid + ask) / 2;
}

Price Orderbook::Spread() const {
    auto bid = BestBid(), ask = BestAsk();
    if (bid == Constants::InvalidPrice || ask == Constants::InvalidPrice)
        return 0;
    return ask - bid;
}

OrderbookSnapshot Orderbook::GetSnapshot() const {
    std::scoped_lock lk{ordersMutex_};
    OrderbookSnapshot snap;
    snap.seqNum       = stats_.seqNum.load();
    snap.timestampNs  = std::chrono::steady_clock::now().time_since_epoch().count();
    snap.lastTradePrice = stats_.lastTradePrice.load();
    snap.lastTradeQty   = stats_.lastTradeQty.load();
    snap.totalVolume    = stats_.totalVolume.load();
    snap.tradeCount     = stats_.totalTrades.load();

    snap.bids.reserve(std::min(bids_.size(), size_t(20)));
    snap.asks.reserve(std::min(asks_.size(), size_t(20)));

    auto calcLevel = [](Price price, const OrderPointers& orders) -> LevelInfo {
        Quantity qty = 0;
        for (const auto& o : orders) qty += o->GetRemainingQuantity();
        return {price, qty, (uint32_t)orders.size()};
    };

    int depth = 0;
    for (const auto& [price, orders] : bids_) {
        snap.bids.push_back(calcLevel(price, orders));
        if (++depth >= 20) break;
    }
    depth = 0;
    for (const auto& [price, orders] : asks_) {
        snap.asks.push_back(calcLevel(price, orders));
        if (++depth >= 20) break;
    }
    return snap;
}
