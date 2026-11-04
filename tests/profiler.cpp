#include "../runtime/profiler.h"
#include <cassert>
#include <string>

int main() {
    const char* path = std::getenv("JETMY_PROFILE_CSV");
    const bool enabled = path && *path;
    {
        Runtime::FrameProfiler profiler(321, 181);
        assert(profiler.Enabled() == enabled);
        for (int frame = 0; frame < 6002; ++frame) {
            profiler.Record(7, "Test pixels", Runtime::FrameProfiler::Clock::now());
            profiler.NextFrame();
        }
        assert(!profiler.Enabled());
    }
    if (enabled) {
        std::ifstream input(path);
        std::string line;
        std::getline(input, line);
        assert(line == "frame,width,height,stage,component,ms");
        int rows = 0;
        while (std::getline(input, line)) {
            assert(line.find(",321,181,7,\"Test pixels\",") != std::string::npos);
            ++rows;
        }
        assert(rows == 6000);
    }
}
