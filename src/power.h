#pragma once

#include <string>

enum class PowerState { Unknown, Charging, NotCharging };

struct PowerSample {
    PowerState state = PowerState::Unknown;
    int percent = -1;        // battery capacity, -1 if unavailable
    std::string status;      // raw sysfs "status" text, for logging
};

// Reads /sys/class/power_supply. Paths are discovered once; call discover()
// again if read() keeps returning Unknown.
class PowerMonitor {
public:
    bool discover();
    PowerSample read() const;
    void print() const;  // dump what was discovered (for --print)

private:
    std::string batteryDir_;
    std::string chargerDir_;
};
