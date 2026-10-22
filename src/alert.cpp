#include "alert.h"

AlertTracker::Event AlertTracker::update(PowerState state, clock::time_point now) {
    if (state == PowerState::NotCharging) {
        steadySince_.reset();
        if (!lostSince_) lostSince_ = now;
        if (!warned_ && now - *lostSince_ >= lossAfter_) {
            warned_ = true;
            return Event::PowerLost;
        }
    } else if (state == PowerState::Charging) {
        lostSince_.reset();
        if (!steadySince_) steadySince_ = now;
        if (warned_ && now - *steadySince_ >= restoreAfter_) {
            warned_ = false;
            return Event::PowerRestored;
        }
    }
    return Event::None;
}
