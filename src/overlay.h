#pragma once

#include <openvr.h>

#include <chrono>
#include <string>

// A small HMD-attached toast. show() draws text with cairo and uploads it via
// SetOverlayRaw; tick() handles fade-out and should be called every ~50 ms.
class Overlay {
public:
    ~Overlay();
    bool init();  // connects to a running SteamVR; false if it is not up
    void show(const std::string& title, const std::string& subtitle);
    void tick();
    bool connected() const { return connected_; }
    void shutdown();

private:
    bool connected_ = false;
    vr::VROverlayHandle_t handle_ = vr::k_ulOverlayHandleInvalid;
    std::chrono::steady_clock::time_point shownAt_;
    bool visible_ = false;
};
