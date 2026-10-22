// Renders the two toasts to PNG with the same code the overlay uses.
// Usage: render-icons <output-dir>
#include <cstdio>
#include <string>

#include "icon.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <output-dir>\n", argv[0]);
        return 2;
    }
    const struct {
        Icon icon;
        const char* name;
    } kIcons[] = {{Icon::Charging, "power-present.png"}, {Icon::NotCharging, "power-lost.png"}};

    for (const auto& i : kIcons) {
        cairo_surface_t* surf = renderIcon(i.icon, 3);  // 3x so it is crisp on a README
        std::string path = std::string(argv[1]) + "/" + i.name;
        if (cairo_surface_write_to_png(surf, path.c_str()) != CAIRO_STATUS_SUCCESS) {
            std::fprintf(stderr, "failed to write %s\n", path.c_str());
            return 1;
        }
        cairo_surface_destroy(surf);
        std::printf("wrote %s\n", path.c_str());
    }
    return 0;
}
