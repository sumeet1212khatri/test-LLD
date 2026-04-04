#pragma once

namespace uber {

class Driver;

class IObserver {
public:
    virtual ~IObserver() = default;

    virtual void onDriverLocationUpdate(Driver& driver) = 0;
};

}  // namespace uber
