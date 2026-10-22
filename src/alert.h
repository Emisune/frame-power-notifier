#pragma once

#include <chrono>
#include <optional>

#include "power.h"

// Decides when to alert. The bank's connection is flaky, so a loss only counts
// once power has been gone continuously for `lossAfter`; a restore is only
// reported after a loss was, and once power has been steady for `restoreAfter`.
// Time is passed in so this can be tested without sleeping.
class AlertTracker {
public:
    using clock = std::chrono::steady_clock;
    enum class Event { None, PowerLost, PowerRestored };

    AlertTracker(clock::duration lossAfter, clock::duration restoreAfter)
        : lossAfter_(lossAfter), restoreAfter_(restoreAfter) {}

    // Unknown samples are ignored (timers keep running).
    Event update(PowerState state, clock::time_point now);

private:
    clock::duration lossAfter_;
    clock::duration restoreAfter_;
    std::optional<clock::time_point> lostSince_;
    std::optional<clock::time_point> steadySince_;
    bool warned_ = false;
};
