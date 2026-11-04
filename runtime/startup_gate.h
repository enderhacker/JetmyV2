#pragma once
#include <cstdint>

namespace Runtime {
enum class StartupResult { Allowed, Declined };
using StartupConfirmation = bool (*)(int step, bool menuMode, void* context);

// Each dialog owns a fresh delay measured with a monotonic millisecond clock.
class ConfirmationDelay {
public:
    static constexpr uint64_t DurationMs = 3000;
    void Start(uint64_t now) { started_ = now; active_ = true; }
    bool Ready(uint64_t now) const { return active_ && now >= started_ && now - started_ >= DurationMs; }
    unsigned RemainingSeconds(uint64_t now) const {
        const uint64_t elapsed = active_ && now >= started_ ? now - started_ : 0;
        return elapsed >= DurationMs ? 0 : static_cast<unsigned>((DurationMs - elapsed + 999) / 1000);
    }
private:
    uint64_t started_ = 0;
    bool active_ = false;
};

// Only two explicit approvals pass. No machine/environment checks are performed.
StartupResult EvaluateStartup(bool menuMode, StartupConfirmation confirmation, void* context);
StartupResult CheckStartup(bool menuMode);
}
