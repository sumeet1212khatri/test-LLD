#pragma once

#include <map>
#include <unordered_map>
#include <list>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <functional>
#include <optional>
#include <chrono>
#include <vector>

#include "Types.h"
#include "Order.h"
#include "Trade.h"

class Orderbook {
public:
    using OnTradeCallback   = std::function<void(const Trade&)>;
    using OnOrderCallback   = std::function<void(const Order&)>;
    using OnSnapshotCallback= std::function<void(const OrderbookSnapshot&)>;

    Orderbook();
    ~Orderbook();
    Orderbook(const Orderbook&)            = delete;
    Orderbook& operator=(const Orderbook&) = delete;
    Orderbook(Orderbook&&)                 = delete;
    Orderbook& operator=(Orderbook&&)      = delete;

    // ─── Core API ─────────────────────────────────────────────────────────────
    Trades   AddOrder   (OrderPointer order);
    void     CancelOrder(OrderId orderId);
    Trades   ModifyOrder(OrderModify order);

    // ─── Query API ────────────────────────────────────────────────────────────
    std::size_t        Size()         const;
    bool               HasOrder(OrderId) const;
    OrderbookSnapshot  GetSnapshot()  const;
    Price              BestBid()      const;
    Price              BestAsk()      const;
    Price              MidPrice()     const;
    Price              Spread()       const;

    // ─── Statistics ───────────────────────────────────────────────────────────
    uint64_t GetTotalOrders()    const noexcept { return stats_.totalOrders.load();   }
    uint64_t GetTotalTrades()    const noexcept { return stats_.totalTrades.load();   }
    uint64_t GetTotalCancels()   const noexcept { return stats_.totalCancels.load();  }
    uint64_t GetTotalVolume()    const noexcept { return stats_.totalVolume.load();   }
    uint64_t GetSequenceNumber() const noexcept { return stats_.seqNum.load();        }

    // ─── Event Hooks ──────────────────────────────────────────────────────────
    void SetOnTrade(OnTradeCallback cb)          { onTrade_     = std::move(cb); }
    void SetOnOrderAdded(OnOrderCallback cb)     { onAdded_     = std::move(cb); }
    void SetOnOrderCancelled(OnOrderCallback cb) { onCancelled_ = std::move(cb); }

    void CheckAndTriggerStops(Price lastTrade);

private:
    struct OrderEntry {
        OrderPointer           order;
        OrderPointers::iterator location;
    };

    struct LevelData {
        Quantity quantity{0};
        uint32_t count{0};
        enum class Action { Add, Remove, Match };
    };

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>>    asks_;
    std::unordered_map<OrderId, OrderEntry>             orders_;
    std::unordered_map<Price, LevelData>                levelData_;
    std::multimap<Price, OrderPointer>                  stopBuys_;
    std::multimap<Price, OrderPointer, std::greater<Price>> stopSells_;

    mutable std::mutex ordersMutex_;
    std::thread        pruneThread_;
    std::condition_variable shutdownCV_;
    std::atomic<bool>  shutdown_{false};

    struct Stats {
        std::atomic<uint64_t> totalOrders{0};
        std::atomic<uint64_t> totalTrades{0};
        std::atomic<uint64_t> totalCancels{0};
        std::atomic<uint64_t> totalVolume{0};
        std::atomic<uint64_t> seqNum{0};
        std::atomic<Price>    lastTradePrice{0};
        std::atomic<Quantity> lastTradeQty{0};
    } stats_;

    OnTradeCallback   onTrade_;
    OnOrderCallback   onAdded_;
    OnOrderCallback   onCancelled_;

    void     PruneGoodForDayOrders();
    void     CancelOrdersInternal(const OrderIds& ids);
    void     CancelOrderInternal (OrderId orderId);

    // BUG FIX: Internal add/match that assumes mutex is already held.
    // Required to fix the deadlock in CheckAndTriggerStops and the race
    // condition in ModifyOrder.
    Trades   AddOrderInternal(OrderPointer order);

    bool     CanMatch     (Side side, Price price) const;
    bool     CanFullyFill (Side side, Price price, Quantity qty) const;

    Trades   MatchOrders  ();
    Trade    MakeTrade    (OrderPointer bid, OrderPointer ask, Quantity qty);

    void     UpdateLevelData(Price price, Quantity qty, LevelData::Action action);
    void     OnOrderAdded      (OrderPointer order);
    void     OnOrderCancelled  (OrderPointer order);
    void     OnOrderMatched    (Price price, Quantity qty, bool fullyFilled);
};
