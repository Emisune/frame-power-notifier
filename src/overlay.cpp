#include "overlay.h"

#include <cairo.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

constexpr int kWidth = 256;
constexpr int kHeight = 128;
constexpr float kWidthMeters = 0.16f;
constexpr double kChargingHoldSeconds = 3.0;
constexpr double kPowerLostHoldSeconds = 6.0;  // the one that needs action
constexpr double kFadeSeconds = 1.0;

void roundedRect(cairo_t* cr, double x, double y, double w, double h, double r) {
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
    cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
    cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
}

void drawBolt(cairo_t* cr, double cx, double cy) {
    cairo_new_path(cr);
    cairo_move_to(cr, cx + 6, cy - 30);
    cairo_line_to(cr, cx - 18, cy + 6);
    cairo_line_to(cr, cx - 2, cy + 6);
    cairo_line_to(cr, cx - 8, cy + 30);
    cairo_line_to(cr, cx + 18, cy - 8);
    cairo_line_to(cr, cx + 2, cy - 8);
    cairo_close_path(cr);
    // Dark outline first so the bolt reads against any fill color.
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_source_rgba(cr, 0.07, 0.07, 0.09, 1);
    cairo_set_line_width(cr, 8);
    cairo_stroke_preserve(cr);
    cairo_set_source_rgba(cr, 1.0, 0.85, 0.15, 1);
    cairo_fill(cr);
}

void drawCross(cairo_t* cr, double cx, double cy) {
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    for (int pass = 0; pass < 2; ++pass) {
        if (pass == 0) {
            cairo_set_source_rgba(cr, 0.07, 0.07, 0.09, 1);
            cairo_set_line_width(cr, 22);
        } else {
            cairo_set_source_rgba(cr, 0.95, 0.25, 0.25, 1);
            cairo_set_line_width(cr, 11);
        }
        cairo_move_to(cr, cx - 34, cy - 28);
        cairo_line_to(cr, cx + 34, cy + 28);
        cairo_move_to(cr, cx + 34, cy - 28);
        cairo_line_to(cr, cx - 34, cy + 28);
        cairo_stroke(cr);
    }
}

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

    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, kWidth, kHeight);
    cairo_t* cr = cairo_create(surf);

    // Rounded dark panel.
    roundedRect(cr, 0, 0, kWidth, kHeight, 22);
    cairo_set_source_rgba(cr, 0.07, 0.07, 0.09, 0.94);
    cairo_fill(cr);

    // Battery body, with a terminal nub on the right.
    const double bx = 40, by = 30, bw = 156, bh = 68;
    const double cx = bx + bw / 2, cy = by + bh / 2;

    // Solid tint inside the body: green = external power, red = gone. This is
    // not a charge level; the Frame can't see the external bank's level.
    const double pad = 7;
    if (icon == Icon::Charging) cairo_set_source_rgba(cr, 0.3, 0.8, 0.4, 0.55);
    else cairo_set_source_rgba(cr, 0.92, 0.28, 0.28, 0.35);
    cairo_rectangle(cr, bx + pad, by + pad, bw - 2 * pad, bh - 2 * pad);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_set_line_width(cr, 6);
    roundedRect(cr, bx, by, bw, bh, 10);
    cairo_stroke(cr);
    roundedRect(cr, bx + bw + 3, cy - 13, 13, 26, 4);
    cairo_fill(cr);

    if (icon == Icon::Charging) drawBolt(cr, cx, cy);
    else drawCross(cr, cx, cy);
    cairo_surface_flush(surf);

    // cairo ARGB32 is premultiplied BGRA in memory on little-endian; OpenVR
    // wants straight RGBA, so swizzle and un-premultiply.
    const unsigned char* src = cairo_image_surface_get_data(surf);
    int stride = cairo_image_surface_get_stride(surf);
    std::vector<unsigned char> rgba(static_cast<size_t>(kWidth) * kHeight * 4);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            const unsigned char* p = src + y * stride + x * 4;
            unsigned char* q = &rgba[(static_cast<size_t>(y) * kWidth + x) * 4];
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
    cairo_destroy(cr);
    cairo_surface_destroy(surf);

    vr::VROverlay()->SetOverlayRaw(handle_, rgba.data(), kWidth, kHeight, 4);
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
