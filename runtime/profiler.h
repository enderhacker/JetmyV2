#pragma once
#include <chrono>
#include <cstdlib>
#include <fstream>

namespace Runtime {
// Opt-in timing data only: never desktop pixels or audio. Bound manual-session
// output to 6000 rendered frames, and buffer writes rather than flushing passes.
class FrameProfiler {
public:
    using Clock = std::chrono::steady_clock;
    FrameProfiler(int width, int height) : width_(width), height_(height) {
        const char* path = std::getenv("JETMY_PROFILE_CSV");
        if (!path || !*path) return;
        file_.open(path);
        if (file_) file_ << "frame,width,height,stage,component,ms\n";
    }
    bool Enabled() const { return file_.is_open() && file_.good() && frame_ < 6000; }
    void Record(int stage, const char* component, Clock::time_point begin) {
        if (!Enabled()) return;
        double ms = std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
        file_ << frame_ << ',' << width_ << ',' << height_ << ',' << stage << ",\""
              << component << "\"," << ms << '\n';
    }
    void NextFrame() { ++frame_; }
private:
    std::ofstream file_;
    int width_, height_;
    unsigned long long frame_ = 0;
};
}
