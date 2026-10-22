#include "power.h"

#include <dirent.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace {

const char* kRoot = "/sys/class/power_supply";

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    std::string s;
    std::getline(f, s);
    return s;
}

}  // namespace

bool PowerMonitor::discover() {
    batteryDir_.clear();
    chargerDir_.clear();
    DIR* d = opendir(kRoot);
    if (!d) return false;
    while (dirent* e = readdir(d)) {
        std::string name = e->d_name;
        if (name == "." || name == "..") continue;
        std::string dir = std::string(kRoot) + "/" + name;
        if (readFile(dir + "/type") == "Battery" && batteryDir_.empty()) {
            batteryDir_ = dir;
        } else if (name.find("charger") != std::string::npos && chargerDir_.empty()) {
            chargerDir_ = dir;
        }
    }
    closedir(d);
    return !batteryDir_.empty() || !chargerDir_.empty();
}

PowerSample PowerMonitor::read() const {
    PowerSample s;
    if (!batteryDir_.empty()) {
        s.status = readFile(batteryDir_ + "/status");
        std::string cap = readFile(batteryDir_ + "/capacity");
        if (!cap.empty()) s.percent = std::atoi(cap.c_str());
    }
    // Observed on a Frame: unplugging drops the charger's "online" to 0 and the
    // battery status to "Discharging" together. A sagging power bank can keep
    // "online" at 1 while the battery still drains, which is also the "swap the
    // bank" case, so a Discharging battery wins. "Not charging" (full battery
    // on a charger) is left to the charger's "online" flag.
    if (s.status == "Discharging") {
        s.state = PowerState::NotCharging;
        return s;
    }
    if (!chargerDir_.empty()) {
        std::string online = readFile(chargerDir_ + "/online");
        if (!online.empty()) {
            s.state = online != "0" ? PowerState::Charging : PowerState::NotCharging;
            return s;
        }
    }
    if (s.status == "Charging" || s.status == "Full") s.state = PowerState::Charging;
    else if (s.status == "Discharging") s.state = PowerState::NotCharging;
    return s;
}

void PowerMonitor::print() const {
    std::printf("battery: %s\ncharger: %s\n",
                batteryDir_.empty() ? "(none)" : batteryDir_.c_str(),
                chargerDir_.empty() ? "(none)" : chargerDir_.c_str());
    PowerSample s = read();
    std::printf("percent=%d status='%s' state=%d\n", s.percent, s.status.c_str(),
                static_cast<int>(s.state));
}
