#include "overlay.h"

#include <cairo.h>

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

constexpr int kWidth = 512;
constexpr int kHeight = 128;
constexpr float kWidthMeters = 0.25f;
constexpr double kHoldSeconds = 3.0;
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

void Overlay::show(const std::string& title, const std::string& subtitle) {
    if (!connected_) return;

    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, kWidth, kHeight);
    cairo_t* cr = cairo_create(surf);

    cairo_set_source_rgba(cr, 0.07, 0.07, 0.09, 0.92);
    cairo_paint(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 44);
    cairo_move_to(cr, 24, 62);
    cairo_show_text(cr, title.c_str());
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 30);
    cairo_set_source_rgba(cr, 0.8, 0.8, 0.85, 1);
    cairo_move_to(cr, 24, 104);
    cairo_show_text(cr, subtitle.c_str());
    cairo_surface_flush(surf);

    // cairo ARGB32 is premultiplied BGRA in memory on little-endian; OpenVR
    // wants RGBA, so swizzle. (Colors here are opaque enough that
    // un-premultiplying is not worth it.)
    const unsigned char* src = cairo_image_surface_get_data(surf);
    int stride = cairo_image_surface_get_stride(surf);
    std::vector<unsigned char> rgba(static_cast<size_t>(kWidth) * kHeight * 4);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            const unsigned char* p = src + y * stride + x * 4;
            unsigned char* q = &rgba[(static_cast<size_t>(y) * kWidth + x) * 4];
            q[0] = p[2];
            q[1] = p[1];
            q[2] = p[0];
            q[3] = p[3];
        }
    }
    cairo_destroy(cr);
    cairo_surface_destroy(surf);

    vr::VROverlay()->SetOverlayRaw(handle_, rgba.data(), kWidth, kHeight, 4);
    vr::VROverlay()->SetOverlayAlpha(handle_, 1.0f);
    vr::VROverlay()->ShowOverlay(handle_);
    shownAt_ = std::chrono::steady_clock::now();
    visible_ = true;
}

void Overlay::tick() {
    if (!connected_ || !visible_) return;
    double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - shownAt_).count();
    if (t >= kHoldSeconds + kFadeSeconds) {
        vr::VROverlay()->HideOverlay(handle_);
        visible_ = false;
    } else if (t > kHoldSeconds) {
        float a = 1.0f - static_cast<float>((t - kHoldSeconds) / kFadeSeconds);
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
