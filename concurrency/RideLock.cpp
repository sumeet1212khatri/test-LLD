#include "RideLock.h"

#include <stdexcept>
#include <utility>

namespace uber {

std::shared_ptr<std::mutex> RideMutexRegistry::mutexFor(const std::string& rideId) {
    std::lock_guard<std::mutex> guard(registryMutex_);
    auto it = mutexes_.find(rideId);
    if (it != mutexes_.end()) {
        return it->second;
    }
    auto mutex = std::make_shared<std::mutex>();
    mutexes_[rideId] = mutex;
    return mutex;
}

RideLock::RideLock(std::mutex& mutex) : lock_(mutex) {}

RideLock::RideLock(const std::shared_ptr<std::mutex>& mutex) : lock_() {
    if (!mutex) {
        throw std::runtime_error("RideLock: null mutex");
    }
    lock_ = std::unique_lock<std::mutex>(*mutex);
}

}  // namespace uber
