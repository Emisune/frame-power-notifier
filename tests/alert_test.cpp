#include <cstdio>
#include <cstdlib>

#include "alert.h"

namespace {

using Event = AlertTracker::Event;
using std::chrono::seconds;

int failures = 0;

void expect(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAIL: %s\n", what);
        ++failures;
    }
}

// Feeds `state` once a second for `secs` samples, advancing `t` before each one,
// so the first sample of a run is 1 s after the previous run's last. N samples
// of a new state therefore span N-1 s: the 121st sample of a loss is the one
// that is 120 s after the first.
struct Run {
    int lost = 0;
    int restored = 0;
};

Run feed(AlertTracker& tr, AlertTracker::clock::time_point& t, PowerState state, int secs) {
    Run r;
    for (int i = 0; i < secs; ++i) {
        t += seconds(1);
        Event e = tr.update(state, t);
        if (e == Event::PowerLost) ++r.lost;
        if (e == Event::PowerRestored) ++r.restored;
    }
    return r;
}

}  // namespace

int main() {
    const auto start = AlertTracker::clock::time_point{} + seconds(1000);
    const PowerState on = PowerState::Charging, off = PowerState::NotCharging;

    {
        AlertTracker tr(seconds(120), seconds(10));
        auto t = start;
        expect(feed(tr, t, on, 60).lost == 0, "steady power never alerts");
        expect(feed(tr, t, off, 120).lost == 0, "119s of loss does not alert yet");
        expect(feed(tr, t, off, 1).lost == 1, "120s of loss alerts once");
        expect(feed(tr, t, off, 300).lost == 0, "does not repeat while still lost");
    }
    {
        AlertTracker tr(seconds(120), seconds(10));
        auto t = start;
        // Flaky bank: 100s lost, 1s back, repeat. Never 120s continuous.
        int lost = 0;
        for (int i = 0; i < 20; ++i) {
            lost += feed(tr, t, off, 100).lost;
            lost += feed(tr, t, on, 1).lost;
        }
        expect(lost == 0, "a blip of power resets the 2-minute window");
    }
    {
        AlertTracker tr(seconds(120), seconds(10));
        auto t = start;
        feed(tr, t, off, 130);  // alerted
        expect(feed(tr, t, on, 10).restored == 0, "9s of power is not yet a restore");
        feed(tr, t, off, 1);    // flap resets the restore timer
        expect(feed(tr, t, on, 10).restored == 0, "flap resets the restore timer");
        expect(feed(tr, t, on, 1).restored == 1, "10s steady power reports a restore");
        expect(feed(tr, t, on, 60).restored == 0, "restore is reported once");
        expect(feed(tr, t, off, 121).lost == 1, "a second loss alerts again");
    }
    {
        AlertTracker tr(seconds(120), seconds(10));
        auto t = start;
        expect(feed(tr, t, on, 60).restored == 0, "no restore without a prior alert");
        feed(tr, t, off, 60);
        feed(tr, t, on, 60);
        expect(feed(tr, t, on, 60).restored == 0, "no restore for a loss that never alerted");
    }
    {
        AlertTracker tr(seconds(120), seconds(10));
        auto t = start;
        feed(tr, t, off, 100);
        feed(tr, t, PowerState::Unknown, 10);  // unreadable sysfs: timer keeps running
        expect(feed(tr, t, off, 11).lost == 1, "Unknown samples do not reset the window");
    }
    {
        AlertTracker tr(seconds(120), seconds(10));
        auto t = start;
        expect(feed(tr, t, off, 121).lost == 1, "starting on battery alerts after 2 minutes");
    }

    if (failures == 0) std::printf("all alert tests passed\n");
    return failures == 0 ? 0 : 1;
}
