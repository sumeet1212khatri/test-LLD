# Uber LLD Engine (C++)

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=c%2B%2B&logoColor=white)
![Design Patterns](https://img.shields.io/badge/Design%20Patterns-Strategy%20%7C%20Observer%20%7C%20Singleton-333333)
![Concurrency](https://img.shields.io/badge/Concurrency-ThreadPool%20%7C%20mutexes-006BFF)

Production-style low-level design for a ride-hailing core: domain models, pricing strategies, matching, dispatch orchestration, concurrency primitives, simulation, and metrics. This repository is intended for learning, interviews, and extension into larger systems.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         main.cpp / CLI                          │
└────────────────────────────┬────────────────────────────────────┘
                             │
        ┌────────────────────┼────────────────────┐
        ▼                    ▼                    ▼
┌───────────────┐   ┌────────────────┐   ┌──────────────────┐
│ ThreadPool    │   │ RideSimulator  │   │ MetricsCollector │
│ (worker pool) │   │ (Bangalore sim)│   │ (singleton stats)│
└───────┬───────┘   └───────┬────────┘   └────────┬─────────┘
        │                   │                     │
        │                   ▼                     │
        │           ┌─────────────────┐           │
        │           │ DispatchSystem  │           │
        │           │ (orchestration) │           │
        │           └────────┬────────┘           │
        │                    │                    │
        ▼                    ▼                    ▼
┌───────────────┐   ┌───────────────┐    ┌───────────────┐
│ RiderService  │   │ MatchingEngine│    │ PricingEngine │
│ DriverService │   │ + selectors │      │ + strategies  │
└───────────────┘   └───────────────┘    └───────────────┘
        │                    │                    │
        ▼                    ▼                    ▼
┌───────────────────────────────────────────────────────┐
│ Models: Rider, Driver, Ride, Location, Payment        │
│ State: RideStateMachine · Concurrency: RideLock       │
└───────────────────────────────────────────────────────┘
```

**Key ideas**

- **Strategy**: `IPricingStrategy` with surge, per-km, and flat-rate implementations; `PricingEngine` swaps strategies at runtime.
- **Observer**: `IObserver` + `DriverService::notifyObservers` on location updates.
- **Singleton**: `MetricsCollector` aggregates simulation statistics behind a mutex.
- **Thread safety**: mutex-protected driver pool in `MatchingEngine`, dispatch mutex in `DispatchSystem`, and RAII `RideLock` for per-ride critical sections.

## How to build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

With Unix Makefiles (after the configure step), you can also run `make -C build -j` if you prefer.

On Windows (Developer PowerShell / MSVC or Ninja):

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## How to run

From the **build** directory (important for the default metrics output path):

```bash
cd build
./uber_engine
```

Optional: pass a custom path for the generated JavaScript (absolute or relative to your current working directory):

```bash
./uber_engine ../frontend/simulation_metrics.js
```

On Windows:

```powershell
cd build
.\uber_engine.exe
```

The binary prints a metrics report to stdout and writes `../frontend/simulation_metrics.js` by default (relative to `build/`) for the dashboard.

## Frontend dashboard

Open `frontend/index.html` in a browser after running the engine so `simulation_metrics.js` is refreshed. The page is a single static file (no bundler): dark theme, glassmorphism cards, animated “rides per minute” bars, and a dot-grid map with normal vs surge coloring.

If you open the HTML before running the engine, the checked-in stub `frontend/simulation_metrics.js` still loads and shows zeros.

**Sample console output (illustrative)**

After `./uber_engine` you should see a block similar to:

```
Simulating 500 rides...
✅ Completed: … | ❌ Cancelled: …
⏱  Avg wait time: … min
🚗 Driver utilization: …%
💥 Surge activated: … times
💰 Avg fare: ₹…
```

Exact numbers depend on scheduling, randomness, and concurrency.

## Repository layout (high level)

- `core/models` — entities (`Rider`, `Driver`, `Ride`, `Location`, `Payment`)
- `core/interfaces` — abstract ports (`IPricingStrategy`, `IDriverSelector`, `IMatchingEngine`, `IObserver`)
- `core/services` — `RiderService`, `DriverService`, `DispatchSystem`
- `engine/pricing` — strategies + `PricingEngine`
- `engine/matching` — `MatchingEngine`, `DriverSelector` implementations
- `engine/state` — `RideStateMachine`
- `concurrency` — `ThreadPool`, `RideLock`
- `simulation` — `RideSimulator`, `MetricsCollector`
- `frontend` — `index.html` + generated `simulation_metrics.js`

## License

Use and modify freely for education and prototyping; there is no warranty for production use without further hardening (persistence, auth, geo indexes, observability, etc.).
