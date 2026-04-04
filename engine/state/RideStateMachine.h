#pragma once

#include <memory>

namespace uber {

class Ride;
enum class RideStatus;

class RideStateMachine {
public:
    static void transition(const std::shared_ptr<Ride>& ride, RideStatus newStatus);
};

}  // namespace uber
