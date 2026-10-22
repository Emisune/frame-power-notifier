#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

#include "overlay.h"
#include "power.h"

namespace {

volatile std::sig_atomic_t gStop = 0;
void onSignal(int) { gStop = 1; }

constexpr auto kPollInterval = std::chrono::milliseconds(1000);
constexpr auto kTickInterval = std::chrono::milliseconds(50);
constexpr auto kReconnectInterval = std::chrono::seconds(3);
// A state must hold this long before we alert, to swallow connector flicker.
constexpr auto kDebounce = std::chrono::milliseconds(1500);

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

    Overlay overlay;
    using clock = std::chrono::steady_clock;

    PowerState stable = PowerState::Unknown;     // last state we alerted on / saw at start
    PowerState pending = PowerState::Unknown;    // candidate new state
    clock::time_point pendingSince;
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
            } else if (stable == PowerState::Unknown) {
                stable = s.state;  // first reading: remember, don't alert
            } else if (s.state == stable) {
                pending = PowerState::Unknown;
            } else if (s.state != pending) {
                pending = s.state;
                pendingSince = now;
            } else if (now - pendingSince >= kDebounce) {
                stable = s.state;
                pending = PowerState::Unknown;
                bool charging = stable == PowerState::Charging;
                std::printf("%s (%d%%, status='%s')\n", charging ? "Charging" : "Not charging",
                            s.percent, s.status.c_str());
                overlay.show(charging ? Overlay::Icon::Charging : Overlay::Icon::NotCharging);
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
