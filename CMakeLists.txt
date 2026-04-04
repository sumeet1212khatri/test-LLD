cmake_minimum_required(VERSION 3.15)

project(uber_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(UBER_SOURCES
    core/models/Location.cpp
    core/models/Payment.cpp
    core/models/Rider.cpp
    core/models/Driver.cpp
    core/models/Ride.cpp
    core/services/RiderService.cpp
    core/services/DriverService.cpp
    core/services/DispatchSystem.cpp
    concurrency/ThreadPool.cpp
    concurrency/RideLock.cpp
    engine/pricing/BasePricingStrategy.cpp
    engine/pricing/SurgePricingStrategy.cpp
    engine/pricing/FlatRatePricingStrategy.cpp
    engine/pricing/PerKmPricingStrategy.cpp
    engine/pricing/PricingEngine.cpp
    engine/matching/DriverSelector.cpp
    engine/matching/MatchingEngine.cpp
    engine/state/RideStateMachine.cpp
    simulation/MetricsCollector.cpp
    simulation/RideSimulator.cpp
)

add_executable(uber_engine main.cpp ${UBER_SOURCES})

target_include_directories(uber_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(uber_engine PRIVATE /W4 /permissive-)
else()
    target_compile_options(uber_engine PRIVATE -Wall -Wextra -Wpedantic)
endif()
