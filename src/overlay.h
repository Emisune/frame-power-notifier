#pragma once

#include <openvr.h>

#include <chrono>
#include <string>

// A small HMD-attached toast: a battery with a bolt (external power) or a cross
// (external power gone). show() draws it with cairo
// and uploads it via SetOverlayRaw; tick() handles fade-out and should be
// called every ~50 ms.
class Overlay {
public:
    enum class Icon { Charging, NotCharging };

    ~Overlay();
    bool init();  // connects to a running SteamVR; false if it is not up
    void show(Icon icon);
    void tick();
    bool connected() const { return connected_; }
    void shutdown();

private:
    bool connected_ = false;
    vr::VROverlayHandle_t handle_ = vr::k_ulOverlayHandleInvalid;
    std::chrono::steady_clock::time_point shownAt_;
    bool visible_ = false;
    double holdSeconds_ = 3.0;
};
