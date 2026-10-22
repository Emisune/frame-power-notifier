#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include "alert.h"
#include "overlay.h"
#include "power.h"

namespace {

volatile std::sig_atomic_t gStop = 0;
void onSignal(int) { gStop = 1; }

constexpr auto kPollInterval = std::chrono::milliseconds(1000);
constexpr auto kTickInterval = std::chrono::milliseconds(50);
constexpr auto kReconnectInterval = std::chrono::seconds(3);
constexpr auto kDefaultLossConfirm = std::chrono::seconds(120);
constexpr auto kRestoreConfirm = std::chrono::seconds(10);

// FRAME_POWER_LOSS_SECONDS shortens the 2-minute window, for testing.
std::chrono::seconds lossConfirm() {
    if (const char* v = std::getenv("FRAME_POWER_LOSS_SECONDS")) {
        long n = std::atol(v);
        if (n > 0) return std::chrono::seconds(n);
    }
    return kDefaultLossConfirm;
}

const char* name(PowerState s) {
    return s == PowerState::Charging ? "power" : s == PowerState::NotCharging ? "battery" : "unknown";
}

}  // namespace

int main(int argc, char** argv) {
    // Under systemd stdout is a pipe and fully buffered; flush per line so the
    // journal shows events as they happen.
    std::setvbuf(stdout, nullptr, _IOLBF, 0);

    PowerMonitor power;
    power.discover();

    if (argc > 1 && std::strcmp(argv[1], "--print") == 0) {
        power.print();
        return 0;
    }
    bool test = argc > 1 && std::strcmp(argv[1], "--test") == 0;

    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);

    using clock = std::chrono::steady_clock;
    Overlay overlay;
    AlertTracker tracker(lossConfirm(), kRestoreConfirm);
    std::printf("alerting after %lds without external power\n",
                static_cast<long>(lossConfirm().count()));

    PowerState lastSeen = PowerState::Unknown;
    clock::time_point nextPoll = clock::now();
    clock::time_point nextReconnect = clock::now();
    int testStage = 0;  // --test: 1 = waiting to show the charging icon
    clock::time_point testNext;

    while (!gStop) {
        auto now = clock::now();

        if (!overlay.connected() && now >= nextReconnect) {
            if (overlay.init()) {
                std::printf("connected to SteamVR\n");
                if (test) {
                    // --test shows both icons, one after the other.
                    overlay.show(Overlay::Icon::NotCharging);
                    testStage = 1;
                    testNext = now + std::chrono::seconds(8);
                }
            } else {
                nextReconnect = now + kReconnectInterval;
            }
        }

        if (now >= nextPoll) {
            nextPoll = now + kPollInterval;
            PowerSample s = power.read();
            if (s.state == PowerState::Unknown) {
                power.discover();  // sysfs entries can appear late
            } else {
                // Log every raw change so a flaky connection shows up in the journal.
                if (s.state != lastSeen) {
                    std::printf("on %s (status='%s', battery %d%%)\n", name(s.state),
                                s.status.c_str(), s.percent);
                    lastSeen = s.state;
                }
                switch (tracker.update(s.state, now)) {
                    case AlertTracker::Event::PowerLost:
                        std::printf("ALERT: external power lost\n");
                        overlay.show(Overlay::Icon::NotCharging);
                        break;
                    case AlertTracker::Event::PowerRestored:
                        std::printf("external power restored\n");
                        overlay.show(Overlay::Icon::Charging);
                        break;
                    case AlertTracker::Event::None:
                        break;
                }
            }
        }

        if (testStage == 1 && now >= testNext) {
            overlay.show(Overlay::Icon::Charging);
            testStage = 0;
        }

        overlay.tick();
        std::this_thread::sleep_for(kTickInterval);
    }

    overlay.shutdown();
    return 0;
}
