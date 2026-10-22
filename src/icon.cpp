#include "icon.h"

#include <cmath>

namespace {

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

cairo_surface_t* renderIcon(Icon icon, int scale) {
    cairo_surface_t* surf =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, kIconWidth * scale, kIconHeight * scale);
    cairo_t* cr = cairo_create(surf);
    cairo_scale(cr, scale, scale);

    // Rounded dark panel.
    roundedRect(cr, 0, 0, kIconWidth, kIconHeight, 22);
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

    cairo_destroy(cr);
    cairo_surface_flush(surf);
    return surf;
}
