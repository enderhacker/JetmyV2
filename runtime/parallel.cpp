#include "parallel.h"
#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>

namespace Runtime {
namespace {
class PixelWorkers {
public:
    PixelWorkers() {
        unsigned cores = std::max(1u, std::thread::hardware_concurrency());
        unsigned threads = std::min(8u, cores > 2 ? cores - 1 : cores);
        // Useful for reproducible profiling and machines with unusual scheduling.
        if (const char* setting = std::getenv("JETMY_RENDER_THREADS"))
            threads = std::clamp(std::atoi(setting), 1, int(threads));
        try {
            for (unsigned i = 1; i < threads; ++i)
                workers_.emplace_back([this, i] { Worker(i); });
        } catch (...) {
            // Keep successfully created workers; low-resource systems still render.
        }
    }
    ~PixelWorkers() {
        { std::lock_guard<std::mutex> lock(mutex_); stopping_ = true; }
        start_.notify_all();
        for (auto& worker : workers_) worker.join();
    }
    void Run(int count, void* context, void (*run)(void*, int, int)) {
        if (workers_.empty() || count < 64) { run(context, 0, count); return; }
        const int threads = int(workers_.size()) + 1;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            count_ = count; context_ = context; run_ = run;
            pending_ = int(workers_.size()); ++generation_;
        }
        start_.notify_all();
        run(context, 0, count / threads);
        std::unique_lock<std::mutex> lock(mutex_);
        done_.wait(lock, [this] { return pending_ == 0; });
    }
private:
    void Worker(unsigned index) {
        unsigned generation = 0;
        std::unique_lock<std::mutex> lock(mutex_);
        for (;;) {
            start_.wait(lock, [&] { return stopping_ || generation_ != generation; });
            if (stopping_) return;
            generation = generation_;
            const int count = count_, threads = int(workers_.size()) + 1;
            auto run = run_; void* context = context_;
            lock.unlock();
            run(context, count * int(index) / threads, count * int(index + 1) / threads);
            lock.lock();
            if (--pending_ == 0) done_.notify_one();
        }
    }
    std::vector<std::thread> workers_;
    std::mutex mutex_;
    std::condition_variable start_, done_;
    bool stopping_ = false;
    unsigned generation_ = 0;
    int count_ = 0, pending_ = 0;
    void* context_ = nullptr;
    void (*run_)(void*, int, int) = nullptr;
};
}
void DispatchPixels(int count, void* context, void (*run)(void*, int, int)) {
    static PixelWorkers workers;
    workers.Run(count, context, run);
}
}
