---
title: HFT Order Book Engine
emoji: ⚡
colorFrom: green
colorTo: cyan
sdk: static
pinned: false
license: mit
short_description: Citadel/HFT-grade C++20 matching engine with 20M order benchmark
---

# ⚡ HFT Order Book Engine — Benchmark Dashboard

A **Citadel / HFT-grade** order book matching engine written in **C++20** with a live benchmark dashboard.

## Features

### Engine (C++20)
- **Price-Time Priority** (FIFO within price level)
- **Order Types**: GTC, FAK, FOK, IOC, Market, PostOnly, Iceberg, StopLimit
- **O(1)** order lookup (unordered_map), **O(log N)** price level access (std::map)
- **Thread-safe** with scoped_lock
- **Stop-limit** order triggering on last trade price
- **Iceberg** peak replenishment
- **Event callbacks** (OnTrade, OnOrderAdded, OnOrderCancelled)
- **20M order benchmark** with full latency histogram
- Memory pool allocator + lock-free SPSC/MPMC queues (included)

### Benchmark Results (cloud VM, no DPDK)
| Metric | Value |
|--------|-------|
| Throughput | ~560K orders/sec |
| p50 latency | ~770 ns |
| p99 latency | ~8.6 µs |
| p99.9 latency | ~14.5 µs |
| Trade rate | ~380K trades/sec |

> On bare-metal with DPDK + kernel bypass: 10–50M orders/sec, p50 < 200 ns

### Frontend Dashboard
- Live benchmark simulation in browser
- Latency distribution chart (log-scale histogram)
- Throughput breakdown donut chart
- Real-time orderbook Level 2 snapshot
- JSON results export
- Configurable presets (HFT, Market Maker, Stress Test)

## Build & Run

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run 20M order benchmark
./benchmark --orders 20000000

# Run tests (24/24 pass)
./run_tests

# JSON output for integration
./benchmark --orders 1000000 --json --quiet
```

## File Structure

```
├── include/
│   ├── Types.h          # Primitive types, enums
│   ├── Order.h          # Order class (GTC/FAK/FOK/Iceberg/etc)
│   ├── Orderbook.h      # Matching engine interface
│   ├── Trade.h          # Trade + OrderModify + Snapshot
│   ├── Benchmark.h      # 20M order benchmark harness
│   ├── LockFreeQueue.h  # SPSC + MPMC lock-free queues
│   └── MemoryPool.h     # Object pool + arena allocator
├── src/
│   ├── Orderbook.cpp    # Matching engine implementation
│   └── main.cpp         # CLI entry point
├── tests/
│   └── test_orderbook.cpp  # 24-test suite (no dependencies)
├── frontend/
│   └── index.html       # Benchmark dashboard
└── CMakeLists.txt
```
