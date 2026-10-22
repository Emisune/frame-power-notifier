#pragma once

#include <cairo.h>

enum class Icon { Charging, NotCharging };

constexpr int kIconWidth = 256;
constexpr int kIconHeight = 128;

// Draws the toast (rounded panel + battery + bolt or cross) and returns an
// ARGB32 surface of kIconWidth*scale by kIconHeight*scale. Caller destroys it.
// Needs only cairo, so it also backs the README screenshots (tools/render_icons).
cairo_surface_t* renderIcon(Icon icon, int scale = 1);
