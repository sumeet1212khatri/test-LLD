#include "RideStateMachine.h"

#include <stdexcept>

#include "../../core/models/Ride.h"

namespace uber {

void RideStateMachine::transition(const std::shared_ptr<Ride>& ride, RideStatus newStatus) {
    if (!ride) {
        throw std::runtime_error("RideStateMachine: ride is null");
    }

    const RideStatus current = ride->status();
    bool valid = false;

    switch (current) {
        case RideStatus::REQUESTED:
            valid = (newStatus == RideStatus::ASSIGNED || newStatus == RideStatus::CANCELLED);
            break;
        case RideStatus::ASSIGNED:
            valid = (newStatus == RideStatus::IN_PROGRESS || newStatus == RideStatus::CANCELLED);
            break;
        case RideStatus::IN_PROGRESS:
            valid = (newStatus == RideStatus::COMPLETED);
            break;
        case RideStatus::COMPLETED:
        case RideStatus::CANCELLED:
            valid = false;
            break;
    }

    if (!valid) {
        throw std::runtime_error("RideStateMachine: invalid ride state transition");
    }

    ride->setStatus(newStatus);
}

}  // namespace uber
