#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <limits>
#include <string>

// ─── Primitive Types ───────────────────────────────────────────────────────────
using Price    = std::int32_t;   // Fixed-point: actual / 100 (e.g. 10050 = $100.50)
using Quantity = std::uint32_t;
using OrderId  = std::uint64_t;
using OrderIds = std::vector<OrderId>;

// ─── Constants ─────────────────────────────────────────────────────────────────
struct Constants {
    static constexpr Price    InvalidPrice    = std::numeric_limits<Price>::min();
    static constexpr Quantity InvalidQuantity = std::numeric_limits<Quantity>::max();
    static constexpr OrderId  InvalidOrderId  = 0;
};

// ─── Enums ─────────────────────────────────────────────────────────────────────
enum class Side : uint8_t { Buy = 0, Sell = 1 };

enum class OrderType : uint8_t {
    GoodTillCancel = 0,
    FillAndKill    = 1,
    FillOrKill     = 2,
    GoodForDay     = 3,
    Market         = 4,
    ImmediateOrCancel = 5,  // NEW: alias for FAK used in HFT
    PostOnly       = 6,     // NEW: maker-only, reject if would cross
    StopLimit      = 7,     // NEW: stop-limit order
    Iceberg        = 8,     // NEW: show only peak quantity
};

enum class OrderStatus : uint8_t {
    New,
    PartiallyFilled,
    Filled,
    Cancelled,
    Rejected,
    Pending,
};

// ─── Trade Info ─────────────────────────────────────────────────────────────────
struct TradeInfo {
    OrderId  orderId;
    Price    price;
    Quantity quantity;
};

// ─── Level Info ─────────────────────────────────────────────────────────────────
struct LevelInfo {
    Price    price;
    Quantity quantity;
    uint32_t orderCount;
};

using LevelInfos = std::vector<LevelInfo>;

;
