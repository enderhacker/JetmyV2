#include "runtime/startup_gate.h"
#include <cassert>
#include <iostream>

#ifdef NDEBUG
#error Startup tests require assertions enabled.
#endif

using namespace Runtime;

namespace {
void TestDelay() {
    ConfirmationDelay delay;
    assert(!delay.Ready(100000)); // Never started cannot approve.
    assert(delay.RemainingSeconds(100000) == 3);
    delay.Start(10000);
    assert(!delay.Ready(10000));
    assert(delay.RemainingSeconds(10000) == 3);
    assert(delay.RemainingSeconds(10999) == 3);
    assert(delay.RemainingSeconds(11000) == 2);
    assert(delay.RemainingSeconds(11999) == 2);
    assert(delay.RemainingSeconds(12000) == 1);
    assert(!delay.Ready(12999));
    assert(delay.RemainingSeconds(12999) == 1);
    assert(delay.Ready(13000));
    assert(delay.RemainingSeconds(13000) == 0);
    assert(delay.Ready(20000)); // A late timer never shortens the required wait.
    assert(delay.RemainingSeconds(20000) == 0);
    assert(!delay.Ready(9999)); // A backward clock cannot underflow into approval.
    assert(delay.RemainingSeconds(9999) == 3);

    delay.Start(20000); // Every new confirmation starts a fresh three seconds.
    assert(!delay.Ready(20000));
    assert(!delay.Ready(22999));
    assert(delay.Ready(23000));
    delay.Start(0); // Zero is a valid clock origin, not an inactive sentinel.
    assert(!delay.Ready(2999));
    assert(delay.Ready(3000));
}

struct Prompts {
    int calls = 0;
    int declineAt = 0;
    bool menuMode = false;
    uint64_t now = 10000;
    uint64_t waited = 0;
};

bool Confirm(int step, bool menuMode, void* context) {
    auto& prompts = *static_cast<Prompts*>(context);
    assert(step == ++prompts.calls);
    assert(menuMode == prompts.menuMode);
    ConfirmationDelay delay;
    delay.Start(prompts.now);
    assert(!delay.Ready(prompts.now));
    // No/cancel is available immediately: it need not wait for the Yes button.
    if (step == prompts.declineAt) return false;
    assert(!delay.Ready(prompts.now + ConfirmationDelay::DurationMs - 1));
    prompts.now += ConfirmationDelay::DurationMs;
    prompts.waited += ConfirmationDelay::DurationMs;
    return delay.Ready(prompts.now);
}

void TestConfirmations() {
    for (bool menuMode : {false, true}) {
        for (int declineAt : {0, 1, 2}) {
            Prompts prompts;
            prompts.declineAt = declineAt;
            prompts.menuMode = menuMode;
            const auto result = EvaluateStartup(menuMode, Confirm, &prompts);
            assert(result == (declineAt ? StartupResult::Declined : StartupResult::Allowed));
            assert(prompts.calls == (declineAt == 1 ? 1 : 2));
            const uint64_t expectedWait = declineAt == 1 ? 0 : (declineAt == 2 ? 3000 : 6000);
            assert(prompts.waited == expectedWait);
        }
        assert(EvaluateStartup(menuMode, nullptr, nullptr) == StartupResult::Declined);
        // A previous approval is not remembered on a later startup call.
        for (int launch = 0; launch < 2; ++launch) {
            Prompts prompts;
            prompts.menuMode = menuMode;
            assert(EvaluateStartup(menuMode, Confirm, &prompts) == StartupResult::Allowed);
            assert(prompts.calls == 2 && prompts.waited == 6000);
        }
    }
}
}

int main() {
    TestDelay();
    TestConfirmations();
    std::cout << "Startup confirmation and three-second delay tests passed.\n"
        "No dialogs, machine probes, effects, or audio were started.\n";
}
