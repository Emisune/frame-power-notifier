#include "overlay.h"

#include <cairo.h>

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

constexpr float kWidthMeters = 0.16f;
constexpr double kChargingHoldSeconds = 3.0;
constexpr double kPowerLostHoldSeconds = 6.0;  // the one that needs action
constexpr double kFadeSeconds = 1.0;

}  // namespace

Overlay::~Overlay() { shutdown(); }

bool Overlay::init() {
    // Background init first so we never launch SteamVR ourselves.
    vr::EVRInitError err = vr::VRInitError_None;
    vr::VR_Init(&err, vr::VRApplication_Background);
    if (err != vr::VRInitError_None) return false;
    vr::VR_Shutdown();

    vr::VR_Init(&err, vr::VRApplication_Overlay);
    if (err != vr::VRInitError_None) return false;

    vr::VROverlayError oerr = vr::VROverlay()->CreateOverlay(
        "sasaken.frame-power-notifier", "Power notifier", &handle_);
    if (oerr != vr::VROverlayError_None) {
        std::fprintf(stderr, "CreateOverlay failed: %d\n", static_cast<int>(oerr));
        vr::VR_Shutdown();
        return false;
    }
    vr::VROverlay()->SetOverlayWidthInMeters(handle_, kWidthMeters);

    // Slightly below and ahead of the HMD, centered.
    vr::HmdMatrix34_t m = {{{1, 0, 0, 0.0f}, {0, 1, 0, -0.10f}, {0, 0, 1, -0.6f}}};
    vr::VROverlay()->SetOverlayTransformTrackedDeviceRelative(
        handle_, vr::k_unTrackedDeviceIndex_Hmd, &m);
    connected_ = true;
    return true;
}

void Overlay::show(Icon icon) {
    if (!connected_) return;

    cairo_surface_t* surf = renderIcon(icon);

    // cairo ARGB32 is premultiplied BGRA in memory on little-endian; OpenVR
    // wants straight RGBA, so swizzle and un-premultiply.
    const unsigned char* src = cairo_image_surface_get_data(surf);
    int stride = cairo_image_surface_get_stride(surf);
    std::vector<unsigned char> rgba(static_cast<size_t>(kIconWidth) * kIconHeight * 4);
    for (int y = 0; y < kIconHeight; ++y) {
        for (int x = 0; x < kIconWidth; ++x) {
            const unsigned char* p = src + y * stride + x * 4;
            unsigned char* q = &rgba[(static_cast<size_t>(y) * kIconWidth + x) * 4];
            unsigned a = p[3];
            auto un = [a](unsigned c) -> unsigned char {
                return a == 0 ? 0 : static_cast<unsigned char>(std::min(255u, c * 255 / a));
            };
            q[0] = un(p[2]);
            q[1] = un(p[1]);
            q[2] = un(p[0]);
            q[3] = static_cast<unsigned char>(a);
        }
    }
    cairo_surface_destroy(surf);

    vr::VROverlay()->SetOverlayRaw(handle_, rgba.data(), kIconWidth, kIconHeight, 4);
    vr::VROverlay()->SetOverlayAlpha(handle_, 1.0f);
    vr::VROverlay()->ShowOverlay(handle_);
    holdSeconds_ = icon == Icon::NotCharging ? kPowerLostHoldSeconds : kChargingHoldSeconds;
    shownAt_ = std::chrono::steady_clock::now();
    visible_ = true;
}

void Overlay::tick() {
    if (!connected_ || !visible_) return;
    double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - shownAt_).count();
    if (t >= holdSeconds_ + kFadeSeconds) {
        vr::VROverlay()->HideOverlay(handle_);
        visible_ = false;
    } else if (t > holdSeconds_) {
        float a = 1.0f - static_cast<float>((t - holdSeconds_) / kFadeSeconds);
        vr::VROverlay()->SetOverlayAlpha(handle_, std::max(0.0f, a));
    }
}

void Overlay::shutdown() {
    if (!connected_) return;
    vr::VROverlay()->HideOverlay(handle_);
    vr::VROverlay()->DestroyOverlay(handle_);
    vr::VR_Shutdown();
    connected_ = false;
    visible_ = false;
}
