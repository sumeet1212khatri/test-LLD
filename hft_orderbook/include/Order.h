#pragma once
#include "Types.h"
#include <memory>
#include <stdexcept>
#include <format>
#include <chrono>

/**
 * @brief HFT-grade Order with all order types including Iceberg, StopLimit, PostOnly.
 *        Minimizes memory footprint (cache-line friendly) and avoids virtual dispatch.
 */
class Order {
public:
    // ─── Full constructor ───────────────────────────────────────────────────────
    Order(OrderType   orderType,
          OrderId     orderId,
          Side        side,
          Price       price,
          Quantity    quantity,
          Quantity    peakQuantity  = 0,    // Iceberg: visible peak
          Price       stopPrice     = Constants::InvalidPrice) // StopLimit trigger
        : orderType_      { orderType   }
        , orderId_        { orderId     }
        , side_           { side        }
        , price_          { price       }
        , stopPrice_      { stopPrice   }
        , initialQty_     { quantity    }
        , remainingQty_   { quantity    }
        , displayQty_     { peakQuantity > 0 ? peakQuantity : quantity }
        , peakQty_        { peakQuantity }
        , status_         { OrderStatus::New }
        , timestamp_      { std::chrono::steady_clock::now().time_since_epoch().count() }
    {}

    // ─── Market order convenience ────────────────────────────────────────────────
    Order(OrderId orderId, Side side, Quantity quantity)
        : Order(OrderType::Market, orderId, side, Constants::InvalidPrice, quantity)
    {}

    // ─── Accessors ───────────────────────────────────────────────────────────────
    OrderId     GetOrderId()          const noexcept { return orderId_;        }
    Side        GetSide()             const noexcept { return side_;           }
    Price       GetPrice()            const noexcept { return price_;          }
    Price       GetStopPrice()        const noexcept { return stopPrice_;      }
    OrderType   GetOrderType()        const noexcept { return orderType_;      }
    OrderStatus GetStatus()           const noexcept { return status_;         }
    Quantity    GetInitialQuantity()  const noexcept { return initialQty_;     }
    Quantity    GetRemainingQuantity()const noexcept { return remainingQty_;   }
    Quantity    GetFilledQuantity()   const noexcept { return initialQty_ - remainingQty_; }
    Quantity    GetDisplayQuantity()  const noexcept { return displayQty_;     }
    Quantity    GetPeakQuantity()     const noexcept { return peakQty_;        }
    int64_t     GetTimestamp()        const noexcept { return timestamp_;      }
    bool        IsFilled()            const noexcept { return remainingQty_ == 0; }
    bool        IsIceberg()           const noexcept { return peakQty_ > 0 && peakQty_ < initialQty_; }

    // ─── Mutators ────────────────────────────────────────────────────────────────
    void Fill(Quantity qty) {
        if (qty > remainingQty_) [[unlikely]]
            throw std::logic_error(std::format(
                "Order {} cannot fill {} > remaining {}", orderId_, qty, remainingQty_));
        remainingQty_ -= qty;
        // Iceberg: replenish display quantity from hidden reserve
        if (IsIceberg()) {
            if (displayQty_ >= qty) {
                displayQty_ -= qty;
            } else {
                displayQty_ = 0;
            }
            if (displayQty_ == 0 && remainingQty_ > 0) {
                displayQty_ = std::min(peakQty_, remainingQty_);
            }
        } else {
            displayQty_ = remainingQty_;
        }
        if (remainingQty_ == 0)        status_ = OrderStatus::Filled;
        else if (qty > 0)              status_ = OrderStatus::PartiallyFilled;
    }

    void Cancel() noexcept { status_ = OrderStatus::Cancelled; }

    void ToGoodTillCancel(Price price) {
        if (orderType_ != OrderType::Market) [[unlikely]]
            throw std::logic_error(std::format(
                "Order {} is not Market type, cannot convert to GTC", orderId_));
        price_     = price;
        orderType_ = OrderType::GoodTillCancel;
    }

    void SetStatus(OrderStatus s) noexcept { status_ = s; }

private:
    // Pack fields carefully for cache efficiency (64 bytes total)
    OrderType   orderType_;
    Side        side_;
    OrderStatus status_;
    uint8_t     pad_[1];
    OrderId     orderId_;       // 8 bytes
    Price       price_;         // 4 bytes
    Price       stopPrice_;     // 4 bytes
    Quantity    initialQty_;    // 4 bytes
    Quantity    remainingQty_;  // 4 bytes
    Quantity    displayQty_;    // 4 bytes (iceberg: visible)
    Quantity    peakQty_;       // 4 bytes (iceberg: peak size)
    int64_t     timestamp_;     // 8 bytes (nanoseconds)
};

using OrderPointer  = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;
