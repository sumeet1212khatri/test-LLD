#pragma once
#include "Types.h"
#include <memory>
#include <stdexcept>
#include <string>
#include <list>
#include <chrono>

class Order {
public:
    Order(OrderType   orderType,
          OrderId     orderId,
          Side        side,
          Price       price,
          Quantity    quantity,
          Quantity    peakQuantity  = 0,
          Price       stopPrice     = Constants::InvalidPrice)
        : orderType_    { orderType    }
        , orderId_      { orderId      }
        , side_         { side         }
        , status_       { OrderStatus::New }
        , price_        { price        }
        , stopPrice_    { stopPrice    }
        , initialQty_   { quantity     }
        , remainingQty_ { quantity     }
        , displayQty_   { peakQuantity > 0 ? peakQuantity : quantity }
        , peakQty_      { peakQuantity }
        , timestamp_    { std::chrono::steady_clock::now().time_since_epoch().count() }
    {}

    // Market order convenience
    Order(OrderId orderId, Side side, Quantity quantity)
        : Order(OrderType::Market, orderId, side, Constants::InvalidPrice, quantity)
    {}

    // ─── Accessors ───────────────────────────────────────────────────────────
    OrderId     GetOrderId()           const noexcept { return orderId_;        }
    Side        GetSide()              const noexcept { return side_;           }
    Price       GetPrice()             const noexcept { return price_;          }
    Price       GetStopPrice()         const noexcept { return stopPrice_;      }
    OrderType   GetOrderType()         const noexcept { return orderType_;      }
    OrderStatus GetStatus()            const noexcept { return status_;         }
    Quantity    GetInitialQuantity()   const noexcept { return initialQty_;     }
    Quantity    GetRemainingQuantity() const noexcept { return remainingQty_;   }
    Quantity    GetFilledQuantity()    const noexcept { return initialQty_ - remainingQty_; }
    Quantity    GetDisplayQuantity()   const noexcept { return displayQty_;     }
    Quantity    GetPeakQuantity()      const noexcept { return peakQty_;        }
    int64_t     GetTimestamp()         const noexcept { return timestamp_;      }
    bool        IsFilled()             const noexcept { return remainingQty_ == 0; }
    bool        IsIceberg()            const noexcept { return peakQty_ > 0 && peakQty_ < initialQty_; }

    // ─── Mutators ────────────────────────────────────────────────────────────
    void Fill(Quantity qty) {
        if (qty > remainingQty_) [[unlikely]]
            throw std::logic_error(
                "Order " + std::to_string(orderId_) +
                " cannot fill " + std::to_string(qty) +
                " > remaining " + std::to_string(remainingQty_));

        remainingQty_ -= qty;

        if (IsIceberg()) {
            if (displayQty_ >= qty) displayQty_ -= qty;
            else                    displayQty_ = 0;
            if (displayQty_ == 0 && remainingQty_ > 0)
                displayQty_ = std::min(peakQty_, remainingQty_);
        } else {
            displayQty_ = remainingQty_;
        }

        if (remainingQty_ == 0) status_ = OrderStatus::Filled;
        else if (qty > 0)       status_ = OrderStatus::PartiallyFilled;
    }

    void Cancel() noexcept { status_ = OrderStatus::Cancelled; }

    void ToGoodTillCancel(Price price) {
        if (orderType_ != OrderType::Market) [[unlikely]]
            throw std::logic_error(
                "Order " + std::to_string(orderId_) +
                " is not Market type, cannot convert to GTC");
        price_     = price;
        orderType_ = OrderType::GoodTillCancel;
    }

    void SetStatus(OrderStatus s) noexcept { status_ = s; }

private:
    // Members declared in initialization order (matches constructor init list)
    // to avoid -Wreorder warnings
    OrderType   orderType_;
    OrderId     orderId_;
    Side        side_;
    OrderStatus status_;
    Price       price_;
    Price       stopPrice_;
    Quantity    initialQty_;
    Quantity    remainingQty_;
    Quantity    displayQty_;
    Quantity    peakQty_;
    int64_t     timestamp_;
};

// BUG FIX: Original defined OrderPointer as Order* (raw pointer).
// All engine code uses shared_ptr semantics (lifetime managed, no manual delete).
// Raw pointer caused "cannot convert shared_ptr<Order> to Order*" errors.
using OrderPointer  = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;
