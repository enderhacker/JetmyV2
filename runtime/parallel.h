#pragma once

namespace Runtime {
// Synchronous, non-nested dispatch. Call only from the render thread, with
// independent output rows/columns; workers must never call GDI or FastRand.
void DispatchPixels(int count, void* context, void (*run)(void*, int, int));
template<class Function> void ParallelFor(int count, Function function) {
    DispatchPixels(count, &function, [](void* context, int begin, int end) {
        auto& fn = *static_cast<Function*>(context);
        for (int i = begin; i < end; ++i) fn(i);
    });
}
}
