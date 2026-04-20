#pragma once
#include "Types.h"
#include "Order.h"
#include <vector>

// ─── Trade ──────────────────────────────────────────────────────────────────────
class Trade {
public:
    Trade(const TradeInfo& bid, const TradeInfo& ask, int64_t tsNs = 0)
        : bidTrade_{bid}, askTrade_{ask}, timestampNs_{tsNs} {}

    const TradeInfo& GetBidTrade()  const noexcept { return bidTrade_;     }
    const TradeInfo& GetAskTrade()  const noexcept { return askTrade_;     }
    int64_t          GetTimestamp() const noexcept { return timestampNs_;  }
    Price            GetPrice()     const noexcept { return askTrade_.price; }
    Quantity         GetQuantity()  const noexcept { return askTrade_.quantity; }

private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
    int64_t   timestampNs_;
};

using Trades = std::vector<Trade>;

// ─── OrderModify ────────────────────────────────────────────────────────────────
class OrderModify {
public:
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
        : orderId_{orderId}, side_{side}, price_{price}, quantity_{quantity} {}

    OrderId  GetOrderId()  const noexcept { return orderId_;  }
    Side     GetSide()     const noexcept { return side_;     }
    Price    GetPrice()    const noexcept { return price_;    }
    Quantity GetQuantity() const noexcept { return quantity_; }

    OrderPointer ToOrderPointer(OrderType type) const {
        return std::make_shared<Order>(type, orderId_, side_, price_, quantity_);
    }

private:
    OrderId  orderId_;
    Side     side_;
    Price    price_;
    Quantity quantity_;
};

// ─── Orderbook Snapshot ─────────────────────────────────────────────────────────
struct OrderbookSnapshot {
    LevelInfos bids;
    LevelInfos asks;
    uint64_t   seqNum;
    int64_t    timestampNs;
    Price      lastTradePrice;
    Quantity   lastTradeQty;
    uint64_t   totalVolume;
    uint64_t   tradeCount;
};
