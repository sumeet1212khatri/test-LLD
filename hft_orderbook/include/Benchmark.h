#pragma once
#include "Orderbook.h"
#include "Types.h"
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <iostream>

struct BenchmarkConfig {
    uint64_t totalOrders     = 20'000'000;
    uint32_t numSymbols      = 1;
    double   cancelRatio     = 0.20;
    double   modifyRatio     = 0.05;
    double   marketRatio     = 0.10;
    double   icebergRatio    = 0.02;
    double   bidAskRatio     = 0.50;
    Price    midPrice        = 10000;
    Price    priceSpread     = 500;
    Quantity minQty          = 1;
    Quantity maxQty          = 1000;
    bool     warmup          = true;
    uint64_t warmupOrders    = 100'000;
    bool     verbose         = true;
    bool     jsonOutput      = false;
    uint32_t latencySamples  = 1'000'000;
};

struct BenchmarkResult {
    uint64_t totalOrders;
    uint64_t totalTrades;
    uint64_t totalCancels;
    uint64_t ordersProcessed;
    double   wallTimeSeconds;
    double   ordersPerSecond;
    double   tradesPerSecond;
    double   avgLatencyNs;
    double   minLatencyNs;
    double   maxLatencyNs;
    double   p50LatencyNs;
    double   p90LatencyNs;
    double   p95LatencyNs;
    double   p99LatencyNs;
    double   p999LatencyNs;
    double   p9999LatencyNs;
    double   stdDevNs;
    uint64_t finalBookDepth;
    std::vector<uint64_t> latencies;

    std::string ToJson() const {
        std::ostringstream o;
        o << std::fixed << std::setprecision(2);
        o << "{\n";
        o << "  \"totalOrders\": "     << totalOrders     << ",\n";
        o << "  \"totalTrades\": "     << totalTrades     << ",\n";
        o << "  \"totalCancels\": "    << totalCancels    << ",\n";
        o << "  \"ordersPerSecond\": " << ordersPerSecond << ",\n";
        o << "  \"tradesPerSecond\": " << tradesPerSecond << ",\n";
        o << "  \"wallTimeSeconds\": " << wallTimeSeconds << ",\n";
        o << "  \"latency\": {\n";
        o << "    \"avgNs\": "    << avgLatencyNs  << ",\n";
        o << "    \"minNs\": "    << minLatencyNs  << ",\n";
        o << "    \"maxNs\": "    << maxLatencyNs  << ",\n";
        o << "    \"stdDevNs\": " << stdDevNs      << ",\n";
        o << "    \"p50Ns\": "    << p50LatencyNs  << ",\n";
        o << "    \"p90Ns\": "    << p90LatencyNs  << ",\n";
        o << "    \"p95Ns\": "    << p95LatencyNs  << ",\n";
        o << "    \"p99Ns\": "    << p99LatencyNs  << ",\n";
        o << "    \"p999Ns\": "   << p999LatencyNs << ",\n";
        o << "    \"p9999Ns\": "  << p9999LatencyNs<< "\n";
        o << "  },\n";
        o << "  \"finalBookDepth\": "  << finalBookDepth  << "\n";
        o << "}";
        return o.str();
    }
};

class Benchmark {
public:
    explicit Benchmark(const BenchmarkConfig& cfg = BenchmarkConfig{}) : cfg_{cfg} {}

    BenchmarkResult Run() {
        Orderbook book;
        std::mt19937_64 rng{42};
        std::uniform_int_distribution<Price>    priceDist{
            cfg_.midPrice - cfg_.priceSpread,
            cfg_.midPrice + cfg_.priceSpread};
        std::uniform_int_distribution<Quantity> qtyDist{cfg_.minQty, cfg_.maxQty};
        std::uniform_real_distribution<double>  chanceDist{0.0, 1.0};

        if (cfg_.warmup) {
            if (cfg_.verbose)
                std::cout << "[BENCH] Warming up with " << cfg_.warmupOrders << " orders...\n";
            for (uint64_t i = 1; i <= cfg_.warmupOrders; ++i) {
                book.AddOrder(std::make_shared<Order>(
                    OrderType::GoodTillCancel, i, Side::Buy, cfg_.midPrice - 10, 100));
            }
        }

        std::vector<uint64_t> latencySamples;
        latencySamples.reserve(cfg_.latencySamples);
        const uint64_t sampleEvery =
            std::max(uint64_t(1), cfg_.totalOrders / cfg_.latencySamples);

        // BUG FIX: Original used vector<OrderId> with erase-by-index, which is
        // O(n) per cancel due to shifting. With liveIds capped at 50k entries
        // and 20% cancel ratio across 20M orders, this caused ~2M O(50k) shifts
        // — a massive hidden cost that inflated benchmark wall time and latency
        // histograms. Replaced with swap-and-pop idiom for O(1) removal.
        std::vector<OrderId> liveIds;
        liveIds.reserve(50000);
        uint64_t orderIdCounter = cfg_.warmupOrders + 1;

        if (cfg_.verbose)
            std::cout << "[BENCH] Starting " << cfg_.totalOrders << " order benchmark...\n";

        auto wallStart = std::chrono::high_resolution_clock::now();

        for (uint64_t i = 0; i < cfg_.totalOrders; ++i) {
            double roll = chanceDist(rng);
            auto t0 = std::chrono::high_resolution_clock::now();

            if (!liveIds.empty() && roll < cfg_.cancelRatio) {
                // O(1) swap-and-pop instead of O(n) erase
                size_t idx = rng() % liveIds.size();
                OrderId id = liveIds[idx];
                liveIds[idx] = liveIds.back();
                liveIds.pop_back();
                book.CancelOrder(id);

            } else if (!liveIds.empty() && roll < cfg_.cancelRatio + cfg_.modifyRatio) {
                size_t idx = rng() % liveIds.size();
                OrderId id = liveIds[idx];
                book.ModifyOrder(OrderModify{id,
                    chanceDist(rng) < 0.5 ? Side::Buy : Side::Sell,
                    priceDist(rng), qtyDist(rng)});

            } else {
                OrderId id = orderIdCounter++;
                Side side = chanceDist(rng) < cfg_.bidAskRatio ? Side::Buy : Side::Sell;
                Price price = priceDist(rng);
                Quantity qty = qtyDist(rng);

                OrderPointer order;
                if (roll > 1.0 - cfg_.marketRatio) {
                    order = std::make_shared<Order>(id, side, qty);
                } else if (roll > 1.0 - cfg_.marketRatio - cfg_.icebergRatio) {
                    Quantity peak = std::max(Quantity(1), qty / 5);
                    order = std::make_shared<Order>(
                        OrderType::Iceberg, id, side, price, qty, peak);
                } else {
                    order = std::make_shared<Order>(
                        OrderType::GoodTillCancel, id, side, price, qty);
                }

                book.AddOrder(order);
                if (!order->IsFilled() &&
                    order->GetStatus() != OrderStatus::Cancelled &&
                    order->GetStatus() != OrderStatus::Rejected &&
                    order->GetOrderType() != OrderType::Market &&
                    liveIds.size() < 50000) {
                    liveIds.push_back(id);
                }
            }

            auto t1 = std::chrono::high_resolution_clock::now();
            if ((i % sampleEvery == 0) && latencySamples.size() < cfg_.latencySamples)
                latencySamples.push_back(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        }

        auto wallEnd = std::chrono::high_resolution_clock::now();
        double wallSec = std::chrono::duration<double>(wallEnd - wallStart).count();

        std::sort(latencySamples.begin(), latencySamples.end());
        auto pct = [&](double p) -> double {
            if (latencySamples.empty()) return 0;
            size_t idx = std::min(
                size_t(p / 100.0 * latencySamples.size()), latencySamples.size() - 1);
            return static_cast<double>(latencySamples[idx]);
        };
        double sum = 0;
        for (auto v : latencySamples) sum += v;
        double avg = latencySamples.empty() ? 0 : sum / latencySamples.size();
        double var = 0;
        for (auto v : latencySamples) var += (v - avg) * (v - avg);
        double stddev = latencySamples.empty() ? 0 : std::sqrt(var / latencySamples.size());

        BenchmarkResult res{};
        res.totalOrders      = cfg_.totalOrders;
        res.totalTrades      = book.GetTotalTrades();
        res.totalCancels     = book.GetTotalCancels();
        res.ordersProcessed  = cfg_.totalOrders;
        res.wallTimeSeconds  = wallSec;
        res.ordersPerSecond  = cfg_.totalOrders / wallSec;
        res.tradesPerSecond  = res.totalTrades / wallSec;
        res.avgLatencyNs     = avg;
        res.stdDevNs         = stddev;
        res.minLatencyNs     = latencySamples.empty() ? 0 : latencySamples.front();
        res.maxLatencyNs     = latencySamples.empty() ? 0 : latencySamples.back();
        res.p50LatencyNs     = pct(50);
        res.p90LatencyNs     = pct(90);
        res.p95LatencyNs     = pct(95);
        res.p99LatencyNs     = pct(99);
        res.p999LatencyNs    = pct(99.9);
        res.p9999LatencyNs   = pct(99.99);
        res.finalBookDepth   = book.Size();
        res.latencies        = latencySamples;

        if (cfg_.verbose) {
            std::cout << "\n══════════════════════════════════════════════\n";
            std::cout << "  HFT ORDER BOOK BENCHMARK RESULTS\n";
            std::cout << "══════════════════════════════════════════════\n";
            std::cout << std::fixed << std::setprecision(2);
            std::cout << "  Orders processed : " << res.ordersProcessed   << "\n";
            std::cout << "  Trades executed  : " << res.totalTrades       << "\n";
            std::cout << "  Cancels          : " << res.totalCancels      << "\n";
            std::cout << "  Wall time        : " << wallSec << " sec\n";
            std::cout << "  Throughput       : " << uint64_t(res.ordersPerSecond) << " orders/sec\n";
            std::cout << "  Trade rate       : " << uint64_t(res.tradesPerSecond) << " trades/sec\n";
            std::cout << "\n  Latency (nanoseconds):\n";
            std::cout << "    avg    : " << avg                << "\n";
            std::cout << "    min    : " << res.minLatencyNs   << "\n";
            std::cout << "    p50    : " << res.p50LatencyNs   << "\n";
            std::cout << "    p90    : " << res.p90LatencyNs   << "\n";
            std::cout << "    p95    : " << res.p95LatencyNs   << "\n";
            std::cout << "    p99    : " << res.p99LatencyNs   << "\n";
            std::cout << "    p99.9  : " << res.p999LatencyNs  << "\n";
            std::cout << "    p99.99 : " << res.p9999LatencyNs << "\n";
            std::cout << "    max    : " << res.maxLatencyNs   << "\n";
            std::cout << "    stddev : " << stddev             << "\n";
            std::cout << "  Final book depth : " << res.finalBookDepth << "\n";
            std::cout << "══════════════════════════════════════════════\n";
        }
        if (cfg_.jsonOutput) std::cout << res.ToJson() << "\n";
        return res;
    }

private:
    BenchmarkConfig cfg_;
};
