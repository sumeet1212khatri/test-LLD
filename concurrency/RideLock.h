#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace uber {

class RideMutexRegistry {
public:
    std::shared_ptr<std::mutex> mutexFor(const std::string& rideId);

private:
    std::mutex registryMutex_;
    std::unordered_map<std::string, std::shared_ptr<std::mutex>> mutexes_;
};

class RideLock {
public:
    explicit RideLock(std::mutex& mutex);
    explicit RideLock(const std::shared_ptr<std::mutex>& mutex);

    RideLock(const RideLock&) = delete;
    RideLock& operator=(const RideLock&) = delete;

    RideLock(RideLock&&) noexcept = default;
    RideLock& operator=(RideLock&&) noexcept = default;

    ~RideLock() = default;

private:
    std::unique_lock<std::mutex> lock_;
};

}  // namespace uber
