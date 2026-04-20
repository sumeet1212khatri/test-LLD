#include "Orderbook.h"
#include <iostream>
#include <vector>
#include <string>
#include <functional>

// ─── Mini test framework ───────────────────────────────────────────────────────
struct TestResult { std::string name; bool passed; std::string msg; };
std::vector<TestResult> gResults;
int gPass = 0, gFail = 0;

void runTest(const std::string& name, std::function<void()> fn) {
    bool ok = true; std::string msg;
    try { fn(); }
    catch (const std::logic_error& e) { ok = false; msg = e.what(); }
    catch (const std::exception& e)   { ok = false; msg = e.what(); }
    catch (...)                        { ok = false; msg = "unknown exception"; }
    gResults.push_back({name, ok, msg});
    if (ok) ++gPass; else { ++gFail; std::cerr << "FAIL: " << name << " — " << msg << "\n"; }
}
#define VERIFY_EQ(a,b) do { if ((a)!=(b)) throw std::logic_error(std::string(#a" != "#b" (got ")+std::to_string(a)+" vs "+std::to_string(b)+")"); } while(0)
#define VERIFY_TRUE(c) do { if (!(c)) throw std::logic_error("Expected true: " #c); } while(0)

// ─── Helpers — use make_shared, no ObjectPool dependency ──────────────────────
static OrderPointer MakeGTC(OrderId id, Side side, Price price, Quantity qty) {
    return std::make_shared<Order>(OrderType::GoodTillCancel, id, side, price, qty);
}
static OrderPointer MakeFAK(OrderId id, Side side, Price price, Quantity qty) {
    return std::make_shared<Order>(OrderType::FillAndKill, id, side, price, qty);
}
static OrderPointer MakeFOK(OrderId id, Side side, Price price, Quantity qty) {
    return std::make_shared<Order>(OrderType::FillOrKill, id, side, price, qty);
}
static OrderPointer MakeMarket(OrderId id, Side side, Quantity qty) {
    return std::make_shared<Order>(id, side, qty);
}
static OrderPointer MakeIceberg(OrderId id, Side side, Price price, Quantity qty, Quantity peak) {
    return std::make_shared<Order>(OrderType::Iceberg, id, side, price, qty, peak);
}
static OrderPointer MakePostOnly(OrderId id, Side side, Price price, Quantity qty) {
    return std::make_shared<Order>(OrderType::PostOnly, id, side, price, qty);
}

int main() {
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║  HFT Orderbook Engine — Test Suite       ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";

    runTest("GTC_BasicMatch", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Sell, 100, 10));
        auto trades = book.AddOrder(MakeGTC(2, Side::Buy, 100, 10));
        VERIFY_EQ(trades.size(), 1u);
        VERIFY_EQ(trades[0].GetQuantity(), 10u);
        VERIFY_EQ(book.Size(), 0u);
    });

    runTest("GTC_PartialFill", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Sell, 100, 5));
        auto trades = book.AddOrder(MakeGTC(2, Side::Buy, 100, 10));
        VERIFY_EQ(trades.size(), 1u);
        VERIFY_EQ(trades[0].GetQuantity(), 5u);
        VERIFY_EQ(book.Size(), 1u);
    });

    runTest("FAK_Hit", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Sell, 100, 10));
        auto trades = book.AddOrder(MakeFAK(2, Side::Buy, 100, 5));
        VERIFY_EQ(trades.size(), 1u);
        VERIFY_EQ(book.Size(), 1u);
    });

    runTest("FAK_Miss", [] {
        Orderbook book;
        auto trades = book.AddOrder(MakeFAK(1, Side::Buy, 100, 5));
        VERIFY_EQ(trades.size(), 0u);
        VERIFY_EQ(book.Size(), 0u);
    });

    runTest("FOK_Hit", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Sell, 100, 20));
        auto trades = book.AddOrder(MakeFOK(2, Side::Buy, 100, 10));
        VERIFY_EQ(trades.size(), 1u);
        VERIFY_EQ(trades[0].GetQuantity(), 10u);
    });

    runTest("FOK_Miss", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Sell, 100, 5));
        auto trades = book.AddOrder(MakeFOK(2, Side::Buy, 100, 10));
        VERIFY_EQ(trades.size(), 0u);
        VERIFY_EQ(book.Size(), 1u);
    });

    runTest("Market_Buy_MultipleLevels", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Sell, 100, 10));
        book.AddOrder(MakeGTC(2, Side::Sell, 101, 10));
        auto trades = book.AddOrder(MakeMarket(3, Side::Buy, 15));
        VERIFY_EQ(trades.size(), 2u);
    });

    runTest("Cancel_Success", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Buy, 100, 10));
        VERIFY_EQ(book.Size(), 1u);
        book.CancelOrder(1);
        VERIFY_EQ(book.Size(), 0u);
    });

    runTest("Cancel_NonExistent", [] {
        Orderbook book;
        book.CancelOrder(999);
        VERIFY_EQ(book.Size(), 0u);
    });

    runTest("Modify_Price_Triggers_Match", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Buy, 99, 10));
        book.AddOrder(MakeGTC(2, Side::Sell, 101, 10));
        VERIFY_EQ(book.Size(), 2u);
        auto trades = book.ModifyOrder(OrderModify{1, Side::Buy, 101, 10});
        VERIFY_EQ(trades.size(), 1u);
        VERIFY_EQ(book.Size(), 0u);
    });

    runTest("Modify_Side_Change", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Buy, 100, 10));
        book.ModifyOrder(OrderModify{1, Side::Sell, 100, 10});
        auto snap = book.GetSnapshot();
        VERIFY_EQ(snap.asks.size(), 1u);
        VERIFY_EQ(snap.bids.size(), 0u);
    });

    runTest("Iceberg_Rests_In_Book", [] {
        Orderbook book;
        book.AddOrder(MakeIceberg(1, Side::Sell, 100, 100, 20));
        auto snap = book.GetSnapshot();
        VERIFY_TRUE(snap.asks.size() > 0);
    });

    runTest("Iceberg_Replenish", [] {
        Orderbook book;
        book.AddOrder(MakeIceberg(1, Side::Sell, 100, 100, 25));
        auto trades = book.AddOrder(MakeGTC(2, Side::Buy, 100, 25));
        VERIFY_EQ(trades.size(), 1u);
        VERIFY_EQ(book.Size(), 1u);
    });

    runTest("PostOnly_Rejected_On_Cross", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Sell, 100, 10));
        auto trades = book.AddOrder(MakePostOnly(2, Side::Buy, 100, 10));
        VERIFY_EQ(trades.size(), 0u);
        VERIFY_EQ(book.Size(), 1u);
    });

    runTest("PostOnly_Accepted_As_Maker", [] {
        Orderbook book;
        book.AddOrder(MakePostOnly(1, Side::Buy, 99, 10));
        VERIFY_EQ(book.Size(), 1u);
    });

    runTest("PriceTimePriority", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Sell, 100, 5));
        book.AddOrder(MakeGTC(2, Side::Sell, 100, 5));
        book.AddOrder(MakeGTC(3, Side::Sell, 101, 5));
        auto trades = book.AddOrder(MakeGTC(4, Side::Buy, 100, 5));
        VERIFY_EQ(trades.size(), 1u);
        VERIFY_EQ(trades[0].GetAskTrade().orderId, 1u);
    });

    runTest("Spread_And_MidPrice", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Buy,  99, 10));
        book.AddOrder(MakeGTC(2, Side::Sell, 101, 10));
        VERIFY_EQ(book.BestBid(), 99);
        VERIFY_EQ(book.BestAsk(), 101);
        VERIFY_EQ(book.Spread(), 2);
        VERIFY_EQ(book.MidPrice(), 100);
    });

    runTest("MultiLevel_Sweep_5Levels", [] {
        Orderbook book;
        for (Price p = 100; p <= 110; ++p)
            book.AddOrder(MakeGTC(p, Side::Sell, p, 10));
        auto trades = book.AddOrder(MakeGTC(200, Side::Buy, 104, 50));
        VERIFY_EQ(trades.size(), 5u);
    });

    runTest("DuplicateOrderId_Rejected", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Buy, 99, 10));
        auto trades = book.AddOrder(MakeGTC(1, Side::Buy, 99, 10));
        VERIFY_EQ(trades.size(), 0u);
        VERIFY_EQ(book.Size(), 1u);
    });

    runTest("Stress_100k_Orders", [] {
        Orderbook book;
        for (uint64_t i = 1; i <= 50000; ++i)
            book.AddOrder(MakeGTC(i, Side::Buy,  100 - (Price)(i%10), 10));
        for (uint64_t i = 50001; i <= 100000; ++i)
            book.AddOrder(MakeGTC(i, Side::Sell, 100 + (Price)(i%10), 10));
        VERIFY_TRUE(book.Size() > 0);
    });

    runTest("EventCallbacks_Trade", [] {
        Orderbook book;
        int tradeCount = 0;
        book.SetOnTrade([&](const Trade&){ ++tradeCount; });
        book.AddOrder(MakeGTC(1, Side::Sell, 100, 10));
        book.AddOrder(MakeGTC(2, Side::Buy, 100, 10));
        VERIFY_EQ(tradeCount, 1);
    });

    runTest("Snapshot_Accuracy", [] {
        Orderbook book;
        book.AddOrder(MakeGTC(1, Side::Buy, 99, 10));
        book.AddOrder(MakeGTC(2, Side::Buy, 98, 20));
        book.AddOrder(MakeGTC(3, Side::Sell, 101, 15));
        auto snap = book.GetSnapshot();
        VERIFY_EQ(snap.bids.size(), 2u);
        VERIFY_EQ(snap.asks.size(), 1u);
        VERIFY_EQ(snap.bids[0].price, 99);
        VERIFY_EQ(snap.bids[0].quantity, 10u);
        VERIFY_EQ(snap.asks[0].price, 101);
    });

    runTest("Stats_TotalTrades", [] {
        Orderbook book;
        for (int i = 1; i <= 10; ++i) {
            book.AddOrder(MakeGTC(i*2-1, Side::Sell, 100, 10));
            book.AddOrder(MakeGTC(i*2,   Side::Buy,  100, 10));
        }
        VERIFY_EQ(book.GetTotalTrades(), 10u);
    });

    runTest("Stats_TotalCancels", [] {
        Orderbook book;
        for (int i = 1; i <= 5; ++i)
            book.AddOrder(MakeGTC(i, Side::Buy, 99, 10));
        for (int i = 1; i <= 5; ++i)
            book.CancelOrder(i);
        VERIFY_EQ(book.GetTotalCancels(), 5u);
        VERIFY_EQ(book.Size(), 0u);
    });

    // ── Print summary ─────────────────────────────────────────────────────────
    std::cout << "\n──────────────────────────────────────────\n";
    for (const auto& r : gResults)
        std::cout << (r.passed ? "  ✓ " : "  ✗ ") << r.name
                  << (r.passed ? "" : " — " + r.msg) << "\n";
    std::cout << "──────────────────────────────────────────\n";
    std::cout << "  PASSED: " << gPass << " / " << (gPass + gFail) << "\n";
    if (gFail > 0) std::cout << "  FAILED: " << gFail << "\n";
    std::cout << "──────────────────────────────────────────\n";
    return gFail > 0 ? 1 : 0;
}
